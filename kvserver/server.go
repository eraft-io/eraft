//
// MIT License

// Copyright (c) 2022 eraft dev group

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//
// This is a single raft group system, you can use this to debug your raftcore
//

package kvserver

import (
	"context"
	"encoding/json"
	"strconv"
	"sync"
	"sync/atomic"
	"time"

	"github.com/eraft-io/eraft/common"
	"github.com/eraft-io/eraft/raftcore"
	pb "github.com/eraft-io/eraft/raftpb"
	"github.com/eraft-io/eraft/storage_eng"
)

var PeersMap = map[int]string{0: ":8088", 1: ":8089", 2: ":8090"}

//
// ./redis-server --nvm-maxcapacity 1 --nvm-dir /root --nvm-threshold 64 --pointer-based-aof yes --appendonly yes --appendfsync always --protected-mode no --port 6379
//
// ./redis-server --nvm-maxcapacity 1 --nvm-dir /root --nvm-threshold 64 --pointer-based-aof yes --appendonly yes --appendfsync always --protected-mode no --port 6380
//
// ./redis-server --nvm-maxcapacity 1 --nvm-dir /root --nvm-threshold 64 --pointer-based-aof yes --appendonly yes --appendfsync always --protected-mode no --port 6381
//

var PMemStorageMap = map[int]string{0: "127.0.0.1:6379", 1: "127.0.0.1:6380", 2: "127.0.0.1:6381"}

const ExecCmdTimeout = 1 * time.Second

type KvServer struct {
	mu      sync.RWMutex
	dead    int32
	Rf      *raftcore.Raft
	applyCh chan *pb.ApplyMsg

	lastApplied int

	stm         StateMachine
	notifyChans map[int]chan *pb.CommandResponse
	stopApplyCh chan interface{}

	pb.UnimplementedRaftServiceServer
}

func MakeKvServer(nodeId int, engType string) *KvServer {
	clientEnds := []*raftcore.RaftClientEnd{}
	for id, addr := range PeersMap {
		newEnd := raftcore.MakeRaftClientEnd(addr, uint64(id))
		clientEnds = append(clientEnds, newEnd)
	}
	newApplyCh := make(chan *pb.ApplyMsg)

	var logDbEng storage_eng.KvStore

	switch engType {
	case "pmem":
		logDbEng = storage_eng.EngineFactory("pmem", PMemStorageMap[nodeId])
	case "leveldb":
		logDbEng = storage_eng.EngineFactory("leveldb", "./data/kv_server"+"/node_"+strconv.Itoa(nodeId))
	}

	newRf := raftcore.MakeRaft(clientEnds, nodeId, logDbEng, newApplyCh, 300, 900)
	kvSvr := &KvServer{Rf: newRf, applyCh: newApplyCh, dead: 0, lastApplied: 0, stm: NewMemKV(), notifyChans: make(map[int]chan *pb.CommandResponse)}
	kvSvr.stopApplyCh = make(chan interface{})

	go kvSvr.ApplingToStm(kvSvr.stopApplyCh)

	return kvSvr
}

func (s *KvServer) RequestVote(ctx context.Context, req *pb.RequestVoteRequest) (*pb.RequestVoteResponse, error) {
	resp := &pb.RequestVoteResponse{}
	// raftcore.PrintDebugLog("HandleRequestVote -> " + req.String())
	s.Rf.HandleRequestVote(req, resp)
	// raftcore.PrintDebugLog("SendRequestVoteResp -> " + resp.String())
	return resp, nil
}

func (s *KvServer) AppendEntries(ctx context.Context, req *pb.AppendEntriesRequest) (*pb.AppendEntriesResponse, error) {
	resp := &pb.AppendEntriesResponse{}
	// raftcore.PrintDebugLog("HandleAppendEntries -> " + req.String())
	s.Rf.HandleAppendEntries(req, resp)
	// raftcore.PrintDebugLog("AppendEntriesResp -> " + resp.String())
	return resp, nil
}

func (s *KvServer) getNotifyChan(index int) chan *pb.CommandResponse {
	if _, ok := s.notifyChans[index]; !ok {
		s.notifyChans[index] = make(chan *pb.CommandResponse, 1)
	}
	return s.notifyChans[index]
}

func (s *KvServer) IsKilled() bool {
	return atomic.LoadInt32(&s.dead) == 1
}

func (s *KvServer) ApplingToStm(done <-chan interface{}) {
	for !s.IsKilled() {
		select {
		case <-done:
			return
		case appliedMsg := <-s.applyCh:
			req := &pb.CommandRequest{}
			if err := json.Unmarshal(appliedMsg.Command, req); err != nil {
				raftcore.PrintDebugLog("Unmarshal CommandRequest err")
				continue
			}
			s.lastApplied = int(appliedMsg.CommandIndex)

			var value string
			switch req.OpType {
			case pb.OpType_OpPut:
				s.stm.Put(req.Key, req.Value)
			case pb.OpType_OpAppend:
				s.stm.Append(req.Key, req.Value)
			case pb.OpType_OpGet:
				value, _ = s.stm.Get(req.Key)
			}

			cmdResp := &pb.CommandResponse{}
			cmdResp.Value = value
			ch := s.getNotifyChan(int(appliedMsg.CommandIndex))
			ch <- cmdResp
		}
	}
}

func (s *KvServer) DoCommand(ctx context.Context, req *pb.CommandRequest) (*pb.CommandResponse, error) {
	// raftcore.PrintDebugLog(fmt.Sprintf("do cmd %s", req.String()))

	cmdResp := &pb.CommandResponse{}

	if req != nil {
		reqBytes, err := json.Marshal(req)
		if err != nil {
			return nil, err
		}
		idx, _, isLeader := s.Rf.Propose(reqBytes)
		if !isLeader {
			cmdResp.ErrCode = common.ErrCodeWrongLeader
			return cmdResp, nil
		}

		s.mu.Lock()
		ch := s.getNotifyChan(idx)
		s.mu.Unlock()

		select {
		case res := <-ch:
			cmdResp.Value = res.Value
		case <-time.After(ExecCmdTimeout):
			cmdResp.ErrCode = common.ErrCodeExecTimeout
			cmdResp.Value = "exec cmd timeout"
		}

		go func() {
			s.mu.Lock()
			delete(s.notifyChans, idx)
			s.mu.Unlock()
		}()

	}

	return cmdResp, nil
}
