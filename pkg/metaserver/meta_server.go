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

package metaserver

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"sync"
	"time"

	common "github.com/eraft-io/eraft/pkg/common"
	eng "github.com/eraft-io/eraft/pkg/engine"

	"github.com/eraft-io/eraft/pkg/core/raft"
	"github.com/eraft-io/eraft/pkg/log"
	pb "github.com/eraft-io/eraft/pkg/protocol"
)

type MetaServer struct {
	mu          sync.Mutex
	rf          *raft.Raft
	applyCh     chan *pb.ApplyMsg
	notifyChans map[int64]chan *pb.ServerGroupMetaConfigResponse
	metaEng     eng.KvStore
	stopApplyCh chan interface{}
	pb.UnimplementedRaftServiceServer
	pb.UnimplementedMetaServiceServer
}

func MakeMetaServer(nodes map[int]string, nodeId int, dataPath string) *MetaServer {
	clientEnds := []*raft.RaftClientEnd{}
	for nodeId, nodeAddr := range nodes {
		newEnd := raft.MakeRaftClientEnd(nodeAddr, uint64(nodeId))
		clientEnds = append(clientEnds, newEnd)
	}
	newApplyCh := make(chan *pb.ApplyMsg)
	newRf := raft.MakeRaft(clientEnds, nodeId, newApplyCh, 500, 1500)
	metaStorage := eng.KvStoreFactory("leveldb", fmt.Sprintf("%s/%d", dataPath, nodeId))
	metaServer := &MetaServer{
		rf:          newRf,
		applyCh:     newApplyCh,
		metaEng:     metaStorage,
		notifyChans: make(map[int64]chan *pb.ServerGroupMetaConfigResponse),
	}
	metaServer.stopApplyCh = make(chan interface{})
	go metaServer.ApplingToSTM(metaServer.stopApplyCh)
	return metaServer
}

// RequestVote for metaserver handle request vote from other metaserver node
func (s *MetaServer) RequestVote(ctx context.Context, req *pb.RequestVoteRequest) (*pb.RequestVoteResponse, error) {
	resp := &pb.RequestVoteResponse{}
	log.MainLogger.Debug().Msgf("handle request vote req: %s", req.String())
	s.rf.HandleRequestVote(req, resp)
	log.MainLogger.Debug().Msgf("send request vote resp: %s", resp.String())
	return resp, nil
}

func (s *MetaServer) AppendEntries(ctx context.Context, req *pb.AppendEntriesRequest) (*pb.AppendEntriesResponse, error) {
	resp := &pb.AppendEntriesResponse{}
	log.MainLogger.Debug().Msgf("handle append entries req: %s", req.String())
	s.rf.HandleAppendEntries(req, resp)
	log.MainLogger.Debug().Msgf("handle append entries resp: " + resp.String())
	return resp, nil
}

func (s *MetaServer) Snapshot(ctx context.Context, req *pb.InstallSnapshotRequest) (*pb.InstallSnapshotResponse, error) {
	resp := &pb.InstallSnapshotResponse{}
	log.MainLogger.Debug().Msgf("handle snapshot: %s", req.String())
	s.rf.HandleInstallSnapshot(req, resp)
	log.MainLogger.Debug().Msgf("handle snapshot resp: %s", resp.String())
	return resp, nil
}

func (s *MetaServer) StopAppling() {
	close(s.applyCh)
}

func (s *MetaServer) getRespNotifyChan(logIndex int64) chan *pb.ServerGroupMetaConfigResponse {
	if _, ok := s.notifyChans[logIndex]; !ok {
		s.notifyChans[logIndex] = make(chan *pb.ServerGroupMetaConfigResponse, 1)
	}
	return s.notifyChans[logIndex]
}

func (s *MetaServer) AddBucket(ctx context.Context, req *pb.AddBucketRequest) (*pb.BucketOpResponse, error) {
	resp := pb.BucketOpResponse{}
	bucketId := common.GenGoogleUUID()
	bucket := &pb.Bucket{
		BucketId:   bucketId,
		BucketName: req.BucketName,
	}
	bucketEncodeKey := EncodeBucketKey(bucketId)
	bucketEncodeVal := EncodeBucket(bucket)
	if err := s.metaEng.Put(bucketEncodeKey, bucketEncodeVal); err != nil {
		resp.ErrCode = pb.ErrCode_PUT_BUCKET_TO_ENG_ERR
	}
	resp.ErrCode = pb.ErrCode_NO_ERR
	return &resp, nil
}

func (s *MetaServer) DelBucket(ctx context.Context, req *pb.DelBucketRequest) (*pb.BucketOpResponse, error) {
	resp := pb.BucketOpResponse{}
	bucketEncodeKey := EncodeBucketKey(req.BucketId)
	if err := s.metaEng.Del(bucketEncodeKey); err != nil {
		resp.ErrCode = pb.ErrCode_DEL_BUCKET_FROM_ENG_ERR
	}
	resp.ErrCode = pb.ErrCode_NO_ERR
	return &resp, nil
}

func (s *MetaServer) ListBuckets(ctx context.Context, in *pb.ListBucketRequest) (*pb.ListBucketsResponse, error) {
	resp := pb.ListBucketsResponse{}
	_, vals, err := s.metaEng.GetPrefixRangeKvs(BUCKET_META_PREFIX)
	if err != nil {
		panic(err.Error())
	}
	resp.Buckets = make([]*pb.Bucket, len(vals))
	for i := 0; i < len(vals); i++ {
		bucket := DecodeBucket(vals[i])
		resp.Buckets = append(resp.Buckets, bucket)
	}
	return &resp, nil
}

func (s *MetaServer) ServerGroupMeta(ctx context.Context, req *pb.ServerGroupMetaConfigRequest) (*pb.ServerGroupMetaConfigResponse, error) {
	log.MainLogger.Debug().Msgf("handle server group meta req: %s", req.String())
	resp := &pb.ServerGroupMetaConfigResponse{}
	reqByteSeq, err := json.Marshal(req)
	if err != nil {
		resp.ErrCode = pb.ErrCode_MARSHAL_SERVER_GROUP_META_REQ_ERR
		return resp, err
	}
	logIndex, _, isLeader := s.rf.Propose(reqByteSeq)
	if !isLeader {
		resp.ErrCode = pb.ErrCode_WRONG_LEADER_ERR
		resp.LeaderId = s.rf.GetLeaderId()
		return resp, nil
	}

	logIndexInt64 := int64(logIndex)
	// make a response chan for sync return result to client
	s.mu.Lock()
	ch := s.getRespNotifyChan(logIndexInt64)
	s.mu.Unlock()
	select {
	case res := <-ch:
		resp.ServerGroupMetas = res.ServerGroupMetas
		resp.ErrCode = pb.ErrCode_NO_ERR
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

func (s *MetaServer) ApplingToSTM(done <-chan interface{}) {
	for {
		select {
		case <-done:
			return
		case appliedMsg := <-s.applyCh:
			req := &pb.ServerGroupMetaConfigRequest{}
			if err := json.Unmarshal(appliedMsg.Command, req); err != nil {
				log.MainLogger.Error().Msgf("unmarshal server group meta config err %s", err.Error())
				continue
			}
			resp := &pb.ServerGroupMetaConfigResponse{}
			// TODO: apply msg to stm
			log.MainLogger.Debug().Msgf("apply op to meta server stm: %s", req.String())
			ch := s.getRespNotifyChan(appliedMsg.CommandIndex)
			ch <- resp
		}
	}
}

func (s *MetaServer) FileBlockMeta(ctx context.Context, req *pb.FileBlockMetaConfigRequest) (*pb.FileBlockMetaConfigResponse, error) {
	return nil, nil
}
