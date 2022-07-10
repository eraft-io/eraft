// Copyright [2022] [WellWood] [wellwood-x@googlegroups.com]

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

// 	http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package blockserver

import (
	"context"
	"errors"
	"fmt"
	"os"
	"sync"
	"time"

	"github.com/eraft-io/eraft/pkg/log"

	"github.com/eraft-io/eraft/pkg/core/raft"
	pb "github.com/eraft-io/eraft/pkg/protocol"
)

type BlockServer struct {
	mu          sync.Mutex
	rf          *raft.Raft
	id          int
	gid         int
	applyCh     chan *pb.ApplyMsg
	notifyChans map[int64]chan *pb.FileBlockOpResponse
	stopApplyCh chan interface{}
	dataPath    string
	pb.UnimplementedRaftServiceServer
	pb.UnimplementedFileBlockServiceServer
}

func MakeBlockServer(nodes map[int]string, nodeId int, groudId int, localDataPath string, metaServerAddrs []string) *BlockServer {
	clientEnds := []*raft.RaftClientEnd{}
	for nodeId, nodeAddr := range nodes {
		newEnd := raft.MakeRaftClientEnd(nodeAddr, uint64(nodeId))
		clientEnds = append(clientEnds, newEnd)
	}
	newApplyCh := make(chan *pb.ApplyMsg)
	newRf := raft.MakeRaft(clientEnds, nodeId, newApplyCh, 500, 1500)
	blockServer := &BlockServer{
		rf:          newRf,
		applyCh:     newApplyCh,
		dataPath:    localDataPath,
		id:          nodeId,
		gid:         groudId,
		notifyChans: make(map[int64]chan *pb.FileBlockOpResponse),
	}
	blockServer.stopApplyCh = make(chan interface{})
	go blockServer.ApplyingToSTM(blockServer.stopApplyCh)
	return blockServer
}

func (s *BlockServer) RequestVote(ctx context.Context, req *pb.RequestVoteRequest) (*pb.RequestVoteResponse, error) {
	resp := &pb.RequestVoteResponse{}
	log.MainLogger.Debug().Msgf("handle request vote req: %s", req.String())
	s.rf.HandleRequestVote(req, resp)
	log.MainLogger.Debug().Msgf("send request vote resp: %s", resp.String())
	return resp, nil
}

func (s *BlockServer) AppendEntries(ctx context.Context, req *pb.AppendEntriesRequest) (*pb.AppendEntriesResponse, error) {
	resp := &pb.AppendEntriesResponse{}
	log.MainLogger.Debug().Msgf("handle append entries req: %s", req.String())
	s.rf.HandleAppendEntries(req, resp)
	log.MainLogger.Debug().Msgf("handle append entries resp: " + resp.String())
	return resp, nil
}

func (s *BlockServer) Snapshot(ctx context.Context, req *pb.InstallSnapshotRequest) (*pb.InstallSnapshotResponse, error) {
	resp := &pb.InstallSnapshotResponse{}
	log.MainLogger.Debug().Msgf("handle snapshot: %s", req.String())
	s.rf.HandleInstallSnapshot(req, resp)
	log.MainLogger.Debug().Msgf("handle snapshot resp: %s", resp.String())
	return resp, nil
}

func (s *BlockServer) StopAppling() {
	close(s.applyCh)
}

func (s *BlockServer) getRespNotifyChan(logIndex int64) chan *pb.FileBlockOpResponse {
	if _, ok := s.notifyChans[logIndex]; !ok {
		s.notifyChans[logIndex] = make(chan *pb.FileBlockOpResponse, 1)
	}
	return s.notifyChans[logIndex]
}

func (s *BlockServer) FileBlockOp(ctx context.Context, req *pb.FileBlockOpRequest) (*pb.FileBlockOpResponse, error) {
	log.MainLogger.Debug().Msgf("handle file block op req: %s", req.String())
	resp := &pb.FileBlockOpResponse{}
	reqByteSeq := EncodeBlockServerRequest(req)
	logIndex, _, isLeader := s.rf.Propose(reqByteSeq)
	if !isLeader {
		resp.ErrCode = pb.ErrCode_WRONG_LEADER_ERR
		resp.LeaderId = s.rf.GetLeaderId()
		return resp, nil
	}
	logIndexInt64 := int64(logIndex)
	s.mu.Lock()
	ch := s.getRespNotifyChan(logIndexInt64)
	s.mu.Unlock()

	select {
	case res := <-ch:
		resp.BlockContent = res.BlockContent
		resp.ErrCode = res.ErrCode
		resp.LeaderId = res.LeaderId
	case <-time.After(time.Second * 10):
		delete(s.notifyChans, logIndexInt64)
		resp.ErrCode = pb.ErrCode_RPC_CALL_TIMEOUT_ERR
		return resp, errors.New("exec time out")
	}

	go func() {
		s.mu.Lock()
		delete(s.notifyChans, logIndexInt64)
		s.mu.Unlock()
	}()

	return resp, nil
}

func (s *BlockServer) ApplyingToSTM(done <-chan interface{}) {
	for {
		select {
		case <-done:
			return
		case appliedMsg := <-s.applyCh:
			req := DecodeBlockServerRequest(appliedMsg.Command)
			resp := &pb.FileBlockOpResponse{}
			switch req.OpType {
			case pb.FileBlockOpType_OP_BLOCK_READ:
				{
					fileBytesSeq, err := os.ReadFile(fmt.Sprintf("%s_%d_%d/%s/%d", s.dataPath, s.gid, s.id, req.FileName, req.FileBlocksMeta.BlockId))
					if err != nil {
						resp.ErrCode = pb.ErrCode_READ_FILE_BLOCK_ERR
					}
					resp.BlockContent = fileBytesSeq
				}
			case pb.FileBlockOpType_OP_BLOCK_WRITE:
				{
					// TODO: check if can serve slot
					if err := os.WriteFile(fmt.Sprintf("%s/%s/%d", s.dataPath, req.FileName, req.FileBlocksMeta.BlockId), req.BlockContent, 0644); err != nil {
						resp.ErrCode = pb.ErrCode_WRITE_FILE_BLOCK_ERR
					}
				}
			}
			ch := s.getRespNotifyChan(appliedMsg.CommandIndex)
			ch <- resp
		}
	}
}
