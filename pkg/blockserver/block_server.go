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
	"sync"

	eng "github.com/eraft-io/eraft/pkg/engine"
	"github.com/eraft-io/eraft/pkg/log"

	"github.com/eraft-io/eraft/pkg/core/raft"
	pb "github.com/eraft-io/eraft/pkg/protocol"
)

type BlockServer struct {
	mu          sync.RWMutex
	rf          *raft.Raft
	applyCh     chan *pb.ApplyMsg
	logEng      eng.KvStore
	stopApplyCh chan interface{}
	pb.UnimplementedRaftServiceServer
	pb.UnimplementedFileBlockServiceServer
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

func (s *BlockServer) WriteFileBlock(ctx context.Context, req *pb.WriteFileBlockRequest) (*pb.WriteFileBlockResponse, error) {
	return nil, nil
}

func (s *BlockServer) ReadFileBlock(ctx context.Context, req *pb.ReadFileBlockRequest) (*pb.ReadFileBlockResponse, error) {
	return nil, nil
}
