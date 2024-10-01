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
	"bytes"
	"context"
	"encoding/gob"
	"encoding/json"
	"fmt"
	"strconv"
	"sync"
	"sync/atomic"
	"time"

	"github.com/eraft-io/eraft/common"
	"github.com/eraft-io/eraft/raftcore"
	pb "github.com/eraft-io/eraft/raftpb"
	"github.com/eraft-io/eraft/storage"
)

var DnsMap = map[int]string{0: "eraft-kvserver-0.eraft-kvserver:8088", 1: "eraft-kvserver-1.eraft-kvserver:8089", 2: "eraft-kvserver-2.eraft-kvserver:8090"}
var PeersMap = map[int]string{0: ":8088", 1: ":8089", 2: ":8090"}

const ExecCmdTimeout = 1 * time.Second

type OperationContext struct {
	MaxAppliedCommandId int64
	LastResponse        *pb.CommandResponse
}

type KvServer struct {
	mu      sync.RWMutex
	dead    int32
	Rf      *raftcore.Raft
	applyCh chan *pb.ApplyMsg

	lastApplied int

	stm            StateMachine
	lastOperations map[int64]OperationContext

	notifyChs   map[int]chan *pb.CommandResponse
	stopApplyCh chan interface{}

	pb.UnimplementedRaftServiceServer
}

func MakeKvServer(nodeId int) *KvServer {
	clientEnds := []*raftcore.RaftClientEnd{}
	for id, addr := range PeersMap {
		newEnd := raftcore.MakeRaftClientEnd(addr, uint64(id))
		clientEnds = append(clientEnds, newEnd)
	}
	newApplyCh := make(chan *pb.ApplyMsg)

	logDbEng, err := storage.MakeLevelDBKvStore("./data/kv_server" + "/node_" + strconv.Itoa(nodeId))
	if err != nil {
		raftcore.PrintDebugLog("boot storage engine err!")
		panic(err)
	}

	newRf := raftcore.MakeRaft(clientEnds, nodeId, logDbEng, newApplyCh, 2000, 6000)
	kvSvr := &KvServer{Rf: newRf, applyCh: newApplyCh, dead: 0, lastApplied: 0, stm: NewMemKV(), notifyChs: make(map[int]chan *pb.CommandResponse)}
	kvSvr.stopApplyCh = make(chan interface{})
	kvSvr.restoreSnapshot(newRf.ReadSnapshot())

	go kvSvr.ApplyingToStm(kvSvr.stopApplyCh)

	return kvSvr
}

func (s *KvServer) restoreSnapshot(snapData []byte) {
	if snapData == nil {
		return
	}
	buf := bytes.NewBuffer(snapData)
	data := gob.NewDecoder(buf)
	var stm MemKV
	if data.Decode(&stm) != nil {
		raftcore.PrintDebugLog("decode stm data error")
	}
	stmBytes, _ := json.Marshal(stm)
	raftcore.PrintDebugLog("recover stm -> " + string(stmBytes))
	s.stm = &stm
}

func (s *KvServer) takeSnapshot(index int) {
	var bytesState bytes.Buffer
	enc := gob.NewEncoder(&bytesState)
	enc.Encode(s.stm)
	// snapshot
	s.Rf.Snapshot(index, bytesState.Bytes())
}

func (s *KvServer) RequestVote(ctx context.Context, req *pb.RequestVoteRequest) (*pb.RequestVoteResponse, error) {
	resp := &pb.RequestVoteResponse{}
	raftcore.PrintDebugLog("HandleRequestVote -> " + req.String())
	s.Rf.HandleRequestVote(req, resp)
	raftcore.PrintDebugLog("SendRequestVoteResp -> " + resp.String())
	return resp, nil
}

func (s *KvServer) AppendEntries(ctx context.Context, req *pb.AppendEntriesRequest) (*pb.AppendEntriesResponse, error) {
	resp := &pb.AppendEntriesResponse{}
	raftcore.PrintDebugLog("HandleAppendEntries -> " + req.String())
	s.Rf.HandleAppendEntries(req, resp)
	raftcore.PrintDebugLog("AppendEntriesResp -> " + resp.String())
	return resp, nil
}

func (s *KvServer) Snapshot(ctx context.Context, req *pb.InstallSnapshotRequest) (*pb.InstallSnapshotResponse, error) {
	resp := &pb.InstallSnapshotResponse{}
	raftcore.PrintDebugLog("HandleSnapshot -> " + req.String())
	s.Rf.HandleInstallSnapshot(req, resp)
	raftcore.PrintDebugLog("HandleSnapshotResp -> " + resp.String())
	return resp, nil
}

func (s *KvServer) getNotifyChan(index int) chan *pb.CommandResponse {
	if _, ok := s.notifyChs[index]; !ok {
		s.notifyChs[index] = make(chan *pb.CommandResponse, 1)
	}
	return s.notifyChs[index]
}

func (s *KvServer) IsKilled() bool {
	return atomic.LoadInt32(&s.dead) == 1
}

func (s *KvServer) ApplyingToStm(done <-chan interface{}) {
	for !s.IsKilled() {
		select {
		case <-done:
			return
		case appliedMsg := <-s.applyCh:
			if appliedMsg.CommandValid {
				s.mu.Lock()
				req := &pb.CommandRequest{}
				if err := json.Unmarshal(appliedMsg.Command, req); err != nil {
					raftcore.PrintDebugLog("Unmarshal CommandRequest err")
					s.mu.Unlock()
					continue
				}
				// Time-out check
				if appliedMsg.CommandIndex <= int64(s.lastApplied) {
					s.mu.Unlock()
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

				if s.Rf.GetLogCount() > 50 {
					s.takeSnapshot(int(appliedMsg.CommandIndex))
				}

				s.mu.Unlock()

			} else if appliedMsg.SnapshotValid {
				raftcore.PrintDebugLog("apply snapshot now")
				s.mu.Lock()
				if s.Rf.CondInstallSnapshot(int(appliedMsg.SnapshotTerm), int(appliedMsg.SnapshotIndex), appliedMsg.Snapshot) {
					s.restoreSnapshot(appliedMsg.Snapshot)
					s.lastApplied = int(appliedMsg.SnapshotIndex)
				}
				s.mu.Unlock()
			}
		}
	}
}

func (s *KvServer) DoCommand(ctx context.Context, req *pb.CommandRequest) (*pb.CommandResponse, error) {
	raftcore.PrintDebugLog(fmt.Sprintf("do cmd %s", req.String()))

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
			delete(s.notifyChs, idx)
			s.mu.Unlock()
		}()

	}

	return cmdResp, nil
}
