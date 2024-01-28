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

package metaserver

import (
	"context"
	"encoding/json"
	"errors"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"github.com/eraft-io/eraft/common"
	"github.com/eraft-io/eraft/logger"
	pb "github.com/eraft-io/eraft/raftpb"
	"github.com/eraft-io/eraft/storage"

	"github.com/eraft-io/eraft/raftcore"
)

type MetaServer struct {
	mu          sync.RWMutex
	dead        int32
	Rf          *raftcore.Raft
	applyCh     chan *pb.ApplyMsg
	stm         ConfigStm
	notifyChans map[int]chan *pb.ConfigResponse
	stopApplyCh chan interface{}

	pb.UnimplementedRaftServiceServer
}

func MakeMetaServer(peerMaps map[int]string, nodeId int) *MetaServer {
	client_ends := []*raftcore.RaftClientEnd{}
	for id, addr := range peerMaps {
		new_end := raftcore.MakeRaftClientEnd(addr, uint64(id))
		client_ends = append(client_ends, new_end)
	}
	newApplyCh := make(chan *pb.ApplyMsg)

	newdb_eng := storage.EngineFactory("leveldb", "./data/db/metanode_"+strconv.Itoa(nodeId))
	logdb_eng := storage.EngineFactory("leveldb", "./data/log/metanode_"+strconv.Itoa(nodeId))

	newRf := raftcore.MakeRaft(client_ends, nodeId, logdb_eng, newApplyCh, 50, 150)
	meta_server := &MetaServer{
		Rf:          newRf,
		applyCh:     newApplyCh,
		dead:        0,
		stm:         NewMemConfigStm(newdb_eng),
		notifyChans: make(map[int]chan *pb.ConfigResponse),
	}

	meta_server.stopApplyCh = make(chan interface{})

	go meta_server.ApplingToStm(meta_server.stopApplyCh)
	return meta_server
}

func (s *MetaServer) StopApply() {
	close(s.applyCh)
}

func (s *MetaServer) getNotifyChan(index int) chan *pb.ConfigResponse {
	if _, ok := s.notifyChans[index]; !ok {
		s.notifyChans[index] = make(chan *pb.ConfigResponse, 1)
	}
	return s.notifyChans[index]
}

func (s *MetaServer) DoConfig(ctx context.Context, req *pb.ConfigRequest) (*pb.ConfigResponse, error) {
	logger.ELogger().Sugar().Debugf("DoConfig %s", req.String())

	cmd_resp := &pb.ConfigResponse{}

	req_bytes, err := json.Marshal(req)
	if err != nil {
		cmd_resp.ErrMsg = err.Error()
		return cmd_resp, err
	}

	index, _, isLeader := s.Rf.Propose(req_bytes)
	if !isLeader {
		cmd_resp.ErrMsg = "is not leader"
		cmd_resp.ErrCode = common.ErrCodeWrongLeader
		cmd_resp.LeaderId = s.Rf.GetLeaderId()
		return cmd_resp, nil
	}

	s.mu.Lock()
	ch := s.getNotifyChan(index)
	s.mu.Unlock()

	select {
	case res := <-ch:
		cmd_resp.Config = res.Config
		cmd_resp.ErrMsg = res.ErrMsg
		cmd_resp.ErrCode = common.ErrCodeNoErr
	case <-time.After(ExecTimeout):
		cmd_resp.ErrMsg = "server exec timeout"
		cmd_resp.ErrCode = common.ErrCodeExecTimeout
		return cmd_resp, errors.New("ExecTimeout")
	}

	go func() {
		s.mu.Lock()
		delete(s.notifyChans, index)
		s.mu.Unlock()
	}()

	return cmd_resp, nil
}

func (s *MetaServer) RequestVote(ctx context.Context, req *pb.RequestVoteRequest) (*pb.RequestVoteResponse, error) {
	resp := &pb.RequestVoteResponse{}
	logger.ELogger().Sugar().Debugf("HandleRequestVote -> " + req.String())
	s.Rf.HandleRequestVote(req, resp)
	logger.ELogger().Sugar().Debugf("SendRequestVoteResp -> " + resp.String())
	return resp, nil
}

func (s *MetaServer) AppendEntries(ctx context.Context, req *pb.AppendEntriesRequest) (*pb.AppendEntriesResponse, error) {
	resp := &pb.AppendEntriesResponse{}
	logger.ELogger().Sugar().Debugf("HandleAppendEntries -> " + req.String())
	s.Rf.HandleAppendEntries(req, resp)
	logger.ELogger().Sugar().Debugf("AppendEntriesResp -> " + resp.String())
	return resp, nil
}

func (s *MetaServer) ApplingToStm(done <-chan interface{}) {
	for !s.IsKilled() {
		select {
		case <-done:
			return
		case appliedMsg := <-s.applyCh:
			req := &pb.ConfigRequest{}
			if err := json.Unmarshal(appliedMsg.Command, req); err != nil {
				logger.ELogger().Sugar().Errorf(err.Error())
				continue
			}
			logger.ELogger().Sugar().Debugf("appling msg -> " + appliedMsg.String())
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
				}
				logger.ELogger().Sugar().Debugf("query configs: " + string(out))
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
			logger.ELogger().Sugar().Debugf("query resp: " + resp.String())
			if err != nil {
				resp.ErrMsg = err.Error()
			}

			ch := s.getNotifyChan(int(appliedMsg.CommandIndex))
			ch <- resp
		}
	}
}

func (s *MetaServer) IsKilled() bool {
	return atomic.LoadInt32(&s.dead) == 1
}
