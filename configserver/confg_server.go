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

package configserver

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"github.com/eraft-io/eraft/common"
	pb "github.com/eraft-io/eraft/raftpb"
	"github.com/eraft-io/eraft/storage_eng"

	"github.com/eraft-io/eraft/raftcore"
)

type ConfigServer struct {
	mu          sync.RWMutex
	dead        int32
	Rf          *raftcore.Raft
	applyCh     chan *pb.ApplyMsg
	stm         ConfigStm
	notifyChans map[int]chan *pb.ConfigResponse
	stopApplyCh chan interface{}

	pb.UnimplementedRaftServiceServer
}

func MakeConfigServer(peerMaps map[int]string, nodeId int) *ConfigServer {
	clientEnds := []*raftcore.RaftClientEnd{}
	for id, addr := range peerMaps {
		newEnd := raftcore.MakeRaftClientEnd(addr, uint64(id))
		clientEnds = append(clientEnds, newEnd)
	}
	newApplyCh := make(chan *pb.ApplyMsg)

	newdbEng := storage_eng.EngineFactory("leveldb", "./conf_data/"+"/node_"+strconv.Itoa(nodeId))

	logDbEng := storage_eng.EngineFactory("leveldb", "./log_data/"+"/configserver/node_"+strconv.Itoa(nodeId))

	newRf := raftcore.MakeRaft(clientEnds, nodeId, logDbEng, newApplyCh, 500, 1500)
	configSvr := &ConfigServer{
		Rf:          newRf,
		applyCh:     newApplyCh,
		dead:        0,
		stm:         NewMemConfigStm(newdbEng),
		notifyChans: make(map[int]chan *pb.ConfigResponse),
	}

	configSvr.stopApplyCh = make(chan interface{})

	go configSvr.ApplingToStm(configSvr.stopApplyCh)
	return configSvr
}

func (s *ConfigServer) StopApply() {
	close(s.applyCh)
}

func (s *ConfigServer) getNotifyChan(index int) chan *pb.ConfigResponse {
	if _, ok := s.notifyChans[index]; !ok {
		s.notifyChans[index] = make(chan *pb.ConfigResponse, 1)
	}
	return s.notifyChans[index]
}

func (s *ConfigServer) DoConfig(ctx context.Context, req *pb.ConfigRequest) (*pb.ConfigResponse, error) {
	raftcore.PrintDebugLog(fmt.Sprintf("DoConfig %s", req.String()))
	cmdResp := &pb.ConfigResponse{}

	reqBytes, err := json.Marshal(req)
	if err != nil {
		cmdResp.ErrMsg = err.Error()
		return cmdResp, err
	}

	index, _, isLeader := s.Rf.Propose(reqBytes)
	if !isLeader {
		cmdResp.ErrMsg = "is not leader"
		cmdResp.ErrCode = common.ErrCodeWrongLeader
		cmdResp.LeaderId = s.Rf.GetLeaderId()
		return cmdResp, nil
	}

	s.mu.Lock()
	ch := s.getNotifyChan(index)
	s.mu.Unlock()

	select {
	case res := <-ch:
		cmdResp.Config = res.Config
		cmdResp.ErrMsg = res.ErrMsg
		cmdResp.ErrCode = common.ErrCodeNoErr
	case <-time.After(ExecTimeout):
		cmdResp.ErrMsg = "server exec timeout"
		cmdResp.ErrCode = common.ErrCodeExecTimeout
		return cmdResp, errors.New("ExecTimeout")
	}

	go func() {
		s.mu.Lock()
		delete(s.notifyChans, index)
		s.mu.Unlock()
	}()

	return cmdResp, nil
}

func (s *ConfigServer) RequestVote(ctx context.Context, req *pb.RequestVoteRequest) (*pb.RequestVoteResponse, error) {
	resp := &pb.RequestVoteResponse{}
	raftcore.PrintDebugLog("HandleRequestVote -> " + req.String())
	s.Rf.HandleRequestVote(req, resp)
	raftcore.PrintDebugLog("SendRequestVoteResp -> " + resp.String())
	return resp, nil
}

func (s *ConfigServer) AppendEntries(ctx context.Context, req *pb.AppendEntriesRequest) (*pb.AppendEntriesResponse, error) {
	resp := &pb.AppendEntriesResponse{}
	raftcore.PrintDebugLog("HandleAppendEntries -> " + req.String())
	s.Rf.HandleAppendEntries(req, resp)
	raftcore.PrintDebugLog("AppendEntriesResp -> " + resp.String())
	return resp, nil
}

func (s *ConfigServer) ApplingToStm(done <-chan interface{}) {
	for !s.IsKilled() {
		select {
		case <-done:
			return
		case appliedMsg := <-s.applyCh:
			req := &pb.ConfigRequest{}
			if err := json.Unmarshal(appliedMsg.Command, req); err != nil {
				raftcore.PrintDebugLog("Unmarshal ConfigRequest err")
				continue
			}
			raftcore.PrintDebugLog("appling msg -> " + appliedMsg.String())

			var conf Config
			var err error
			resp := &pb.ConfigResponse{}
			switch req.OpType {
			case pb.ConfigOpType_OpJoin:
				groups := map[int][]string{}
				for gid, serveraddrs := range req.Servers {
					groups[int(gid)] = strings.Split(serveraddrs, ",")
				}
				err = s.stm.Join(groups)
			case pb.ConfigOpType_OpLeave:
				gids := []int{}
				for _, id := range req.Gids {
					gids = append(gids, int(id))
				}
				err = s.stm.Leave(gids)
			case pb.ConfigOpType_OpMove:
				err = s.stm.Move(int(req.BucketId), int(req.Gid))
			case pb.ConfigOpType_OpQuery:
				conf, err = s.stm.Query(int(req.ConfigVersion))
				if err != nil {
					resp.ErrMsg = err.Error()
				}
				out, err := json.Marshal(conf)
				if err != nil {
					resp.ErrMsg = err.Error()
					// panic(err)
				}
				raftcore.PrintDebugLog("query configs: " + string(out))
				resp.Config = &pb.ServerConfig{}
				resp.Config.ConfigVersion = int64(conf.Version)
				for _, sd := range conf.Buckets {
					resp.Config.Buckets = append(resp.Config.Buckets, int64(sd))
				}
				resp.Config.Groups = make(map[int64]string)
				for gid, servers := range conf.Groups {
					resp.Config.Groups[int64(gid)] = strings.Join(servers, ",")
				}
			}
			raftcore.PrintDebugLog("query resp: " + resp.String())
			if err != nil {
				resp.ErrMsg = err.Error()
			}

			ch := s.getNotifyChan(int(appliedMsg.CommandIndex))
			ch <- resp
		}
	}
}

func (s *ConfigServer) IsKilled() bool {
	return atomic.LoadInt32(&s.dead) == 1
}
