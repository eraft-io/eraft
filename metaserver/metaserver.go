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
	notifyChs   map[int]chan *pb.ConfigResponse
	stopApplyCh chan interface{}

	pb.UnimplementedRaftServiceServer
}

func MakeMetaServer(peerMaps map[int]string, nodeId int) *MetaServer {
	clientEnds := []*raftcore.RaftClientEnd{}
	for id, addr := range peerMaps {
		newEnd := raftcore.MakeRaftClientEnd(addr, uint64(id))
		clientEnds = append(clientEnds, newEnd)
	}
	newApplyCh := make(chan *pb.ApplyMsg)

	newDBEng := storage.EngineFactory("leveldb", "./data/db/metanode_"+strconv.Itoa(nodeId))
	logDBEng := storage.EngineFactory("leveldb", "./data/log/metanode_"+strconv.Itoa(nodeId))

	newRf := raftcore.MakeRaft(clientEnds, nodeId, logDBEng, newApplyCh, 50, 150)
	metaServer := &MetaServer{
		Rf:        newRf,
		applyCh:   newApplyCh,
		dead:      0,
		stm:       NewMemConfigStm(newDBEng),
		notifyChs: make(map[int]chan *pb.ConfigResponse),
	}

	metaServer.stopApplyCh = make(chan interface{})

	go metaServer.ApplyingToStm(metaServer.stopApplyCh)
	return metaServer
}

func (s *MetaServer) StopApply() {
	close(s.applyCh)
}

func (s *MetaServer) getNotifyChan(index int) chan *pb.ConfigResponse {
	if _, ok := s.notifyChs[index]; !ok {
		s.notifyChs[index] = make(chan *pb.ConfigResponse, 1)
	}
	return s.notifyChs[index]
}

func (s *MetaServer) DoConfig(ctx context.Context, req *pb.ConfigRequest) (*pb.ConfigResponse, error) {
	logger.ELogger().Sugar().Debugf("DoConfig %s", req.String())

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
		delete(s.notifyChs, index)
		s.mu.Unlock()
	}()

	return cmdResp, nil
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

func (s *MetaServer) ApplyingToStm(done <-chan interface{}) {
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
				for gid, serverAddrs := range req.Servers {
					groups[int(gid)] = strings.Split(serverAddrs, ",")
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
