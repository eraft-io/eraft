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

package shardkvserver

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

	"github.com/eraft-io/mit6.824lab2product/common"
	"github.com/eraft-io/mit6.824lab2product/configserver"
	pb "github.com/eraft-io/mit6.824lab2product/raftpb"
	"github.com/eraft-io/mit6.824lab2product/storage_eng"

	"github.com/eraft-io/mit6.824lab2product/raftcore"
)

type ShardKV struct {
	mu      sync.RWMutex
	dead    int32
	rf      *raftcore.Raft
	applyCh chan *pb.ApplyMsg
	gid_    int
	cvCli   *configserver.CfgCli

	lastApplied int
	lastConfig  configserver.Config
	curConfig   configserver.Config

	stm map[int]*Bucket

	dbEng storage_eng.KvStore

	notifyChans map[int]chan *pb.CommandResponse

	stopApplyCh chan interface{}

	pb.UnimplementedRaftServiceServer
}

//
// MakeShardKVServer make a new shard kv server
// peerMaps: init peer map in the raft group
// nodeId: the peer's nodeId in the raft group
// gid: the node's raft group id
// configServerAddr: config server addr (leader addr, need to optimized into config server peer map)
//
func MakeShardKVServer(peerMaps map[int]string, nodeId int, gid int, configServerAddrs string) *ShardKV {
	clientEnds := []*raftcore.RaftClientEnd{}
	for id, addr := range peerMaps {
		newEnd := raftcore.MakeRaftClientEnd(addr, uint64(id))
		clientEnds = append(clientEnds, newEnd)
	}
	newApplyCh := make(chan *pb.ApplyMsg)

	logDbEng := storage_eng.EngineFactory("leveldb", "./log_data/shard_svr/group_"+strconv.Itoa(gid)+"/node_"+strconv.Itoa(nodeId))
	newRf := raftcore.MakeRaft(clientEnds, nodeId, logDbEng, newApplyCh, 1000, 3000)
	newdbEng := storage_eng.EngineFactory("leveldb", "./data/group_"+strconv.Itoa(gid)+"/node_"+strconv.Itoa(nodeId))

	shardKv := &ShardKV{
		dead:        0,
		rf:          newRf,
		applyCh:     newApplyCh,
		gid_:        gid,
		cvCli:       configserver.MakeCfgSvrClient(common.UN_UNSED_TID, strings.Split(configServerAddrs, ",")),
		lastApplied: 0,
		curConfig:   configserver.DefaultConfig(),
		lastConfig:  configserver.DefaultConfig(),
		stm:         make(map[int]*Bucket),
		dbEng:       newdbEng,
		notifyChans: map[int]chan *pb.CommandResponse{},
	}

	shardKv.initStm(shardKv.dbEng)

	shardKv.curConfig = *shardKv.cvCli.Query(-1)
	shardKv.lastConfig = *shardKv.cvCli.Query(-1)

	shardKv.stopApplyCh = make(chan interface{})

	// start applier
	go shardKv.ApplingToStm(shardKv.stopApplyCh)

	go shardKv.ConfigAction()

	return shardKv
}

//
// CloseApply close the stopApplyCh to stop commit entries apply
//
func (s *ShardKV) CloseApply() {
	close(s.stopApplyCh)
}

func (s *ShardKV) GetRf() *raftcore.Raft {
	return s.rf
}

//
// ConfigAction  sync the config action from configserver
//
func (s *ShardKV) ConfigAction() {
	for !s.IsKilled() {
		if _, isLeader := s.rf.GetState(); isLeader {
			raftcore.PrintDebugLog("10s timeout into config action")
			s.mu.RLock()
			canPerformNextConf := true
			for _, bucket := range s.stm {
				if bucket.Status != Runing {
					canPerformNextConf = false
					raftcore.PrintDebugLog("cano't perform next conf")
					break
				}
			}
			if canPerformNextConf {
				raftcore.PrintDebugLog("can perform next conf")
			}
			curConfVersion := s.curConfig.Version
			s.mu.RUnlock()
			if canPerformNextConf {
				nextConfig := s.cvCli.Query(int64(curConfVersion) + 1)
				nextCfBytes, _ := json.Marshal(nextConfig)
				curCfBytes, _ := json.Marshal(s.curConfig)
				raftcore.PrintDebugLog("next config -> " + string(nextCfBytes))
				raftcore.PrintDebugLog("cur config -> " + string(curCfBytes))
				if nextConfig.Version == curConfVersion+1 {
					req := &pb.CommandRequest{}
					nextCfBytes, _ := json.Marshal(nextConfig)
					raftcore.PrintDebugLog("can perform next conf -> " + string(nextCfBytes))
					req.Context = nextCfBytes
					req.OpType = pb.OpType_OpConfigChange
					reqBytes, _ := json.Marshal(req)
					idx, _, isLeader := s.rf.Propose(reqBytes)
					if !isLeader {
						return
					}
					s.mu.Lock()
					ch := s.getNotifyChan(idx)
					s.mu.Unlock()

					cmdResp := &pb.CommandResponse{}

					select {
					case res := <-ch:
						cmdResp.Value = res.Value
					case <-time.After(configserver.ExecTimeout):
					}

					raftcore.PrintDebugLog("propose config change ok")

					go func() {
						s.mu.Lock()
						delete(s.notifyChans, idx)
						s.mu.Unlock()
					}()
				}
			}
		}
		time.Sleep(time.Second * 5)
	}
}

func (s *ShardKV) CanServe(bucketId int) bool {
	return s.curConfig.Buckets[bucketId] == s.gid_ && (s.stm[bucketId].Status == Runing)
}

func (s *ShardKV) getNotifyChan(index int) chan *pb.CommandResponse {
	if _, ok := s.notifyChans[index]; !ok {
		s.notifyChans[index] = make(chan *pb.CommandResponse, 1)
	}
	return s.notifyChans[index]
}

func (s *ShardKV) IsKilled() bool {
	return atomic.LoadInt32(&s.dead) == 1
}

//
// DoCommand do client put get command
//
func (s *ShardKV) DoCommand(ctx context.Context, req *pb.CommandRequest) (*pb.CommandResponse, error) {
	raftcore.PrintDebugLog(fmt.Sprintf("do cmd %s", req.String()))

	cmdResp := &pb.CommandResponse{}

	if !s.CanServe(common.Key2BucketID(req.Key)) {
		cmdResp.ErrCode = common.ErrCodeWrongGroup
		return cmdResp, nil
	}
	reqBytes, err := json.Marshal(req)
	if err != nil {
		return nil, err
	}
	// propose to raft
	idx, _, isLeader := s.rf.Propose(reqBytes)
	if !isLeader {
		cmdResp.ErrCode = common.ErrCodeWrongLeader
		cmdResp.LeaderId = s.GetRf().GetLeaderId()
		return cmdResp, nil
	}

	s.mu.Lock()
	ch := s.getNotifyChan(idx)
	s.mu.Unlock()

	select {
	case res := <-ch:
		if res != nil {
			cmdResp.ErrCode = common.ErrCodeNoErr
			cmdResp.Value = res.Value
		}
	case <-time.After(configserver.ExecTimeout):
		return cmdResp, errors.New("ExecTimeout")
	}

	go func() {
		s.mu.Lock()
		delete(s.notifyChans, idx)
		s.mu.Unlock()
	}()

	return cmdResp, nil
}

//
// ApplingToStm  apply the commit operation to state machine
//
func (s *ShardKV) ApplingToStm(done <-chan interface{}) {
	for !s.IsKilled() {
		select {
		case <-done:
			return
		case appliedMsg := <-s.applyCh:
			raftcore.PrintDebugLog("appling msg -> " + appliedMsg.String())
			req := &pb.CommandRequest{}
			if err := json.Unmarshal(appliedMsg.Command, req); err != nil {
				raftcore.PrintDebugLog("Unmarshal CommandRequest err")
				continue
			}
			s.lastApplied = int(appliedMsg.CommandIndex)

			cmdResp := &pb.CommandResponse{}
			value := ""
			var err error
			switch req.OpType {
			// Normal Op
			case pb.OpType_OpPut:
				bucket_id := common.Key2BucketID(req.Key)
				if s.CanServe(bucket_id) {
					raftcore.PrintDebugLog("put " + req.Key + " value " + req.Value + " to bucket " + strconv.Itoa(bucket_id))
					s.stm[bucket_id].Put(req.Key, req.Value)
				}
			case pb.OpType_OpAppend:
				bucket_id := common.Key2BucketID(req.Key)
				if s.CanServe(bucket_id) {
					s.stm[bucket_id].Append(req.Key, req.Value)
				}
			case pb.OpType_OpGet:
				bucket_id := common.Key2BucketID(req.Key)
				if s.CanServe(bucket_id) {
					value, err = s.stm[bucket_id].Get(req.Key)
					raftcore.PrintDebugLog("get " + req.Key + " value " + value + " from bucket " + strconv.Itoa(bucket_id))
				}
				cmdResp.Value = value
			case pb.OpType_OpConfigChange:
				nextConfig := &configserver.Config{}
				json.Unmarshal(req.Context, nextConfig)
				if nextConfig.Version == s.curConfig.Version+1 {
					for i := 0; i < common.NBuckets; i++ {
						if s.curConfig.Buckets[i] != s.gid_ && nextConfig.Buckets[i] == s.gid_ {
							gid := s.curConfig.Buckets[i]
							if gid != 0 {
								s.stm[i].Status = Stopped
							}
						}
						if s.curConfig.Buckets[i] == s.gid_ && nextConfig.Buckets[i] != s.gid_ {
							gid := nextConfig.Buckets[i]
							if gid != 0 {
								s.stm[i].Status = Stopped
							}
						}
					}
					s.lastConfig = s.curConfig
					s.curConfig = *nextConfig
					cfBytes, _ := json.Marshal(s.curConfig)
					raftcore.PrintDebugLog("applied config to server -> " + string(cfBytes))
				}
			case pb.OpType_OpDeleteBuckets:
				bucketOpReqs := &pb.BucketOperationRequest{}
				json.Unmarshal(req.Context, bucketOpReqs)
				for _, bid := range bucketOpReqs.BucketIds {
					s.stm[int(bid)].deleteBucketData()
					raftcore.PrintDebugLog("del buckets data list" + strconv.Itoa(int(bid)))
				}
			case pb.OpType_OpInsertBuckets:
				bucketOpReqs := &pb.BucketOperationRequest{}
				json.Unmarshal(req.Context, bucketOpReqs)
				bucketDatas := &BucketDatasVo{}
				json.Unmarshal(bucketOpReqs.BucketsDatas, bucketDatas)
				for bucket_id, kvs := range bucketDatas.Datas {
					s.stm[bucket_id] = NewBucket(s.dbEng, bucket_id)
					for k, v := range kvs {
						s.stm[bucket_id].Put(k, v)
						raftcore.PrintDebugLog("insert kv data to buckets k -> " + k + " v-> " + v)
					}
				}
			}
			if err != nil {
				raftcore.PrintDebugLog(err.Error())
			}

			ch := s.getNotifyChan(int(appliedMsg.CommandIndex))
			ch <- cmdResp
		}
	}
}

//
// init the status machine
//
func (s *ShardKV) initStm(eng storage_eng.KvStore) {
	for i := 0; i < common.NBuckets; i++ {
		if _, ok := s.stm[i]; !ok {
			s.stm[i] = NewBucket(eng, i)
		}
	}
}

//
// rpc interface
//
func (s *ShardKV) RequestVote(ctx context.Context, req *pb.RequestVoteRequest) (*pb.RequestVoteResponse, error) {
	resp := &pb.RequestVoteResponse{}
	raftcore.PrintDebugLog("handle request vote -> " + req.String())
	s.rf.HandleRequestVote(req, resp)
	raftcore.PrintDebugLog("send request vote -> " + resp.String())
	return resp, nil
}

//
// rpc interface
//
func (s *ShardKV) AppendEntries(ctx context.Context, req *pb.AppendEntriesRequest) (*pb.AppendEntriesResponse, error) {
	resp := &pb.AppendEntriesResponse{}
	raftcore.PrintDebugLog("handle append entry -> " + req.String())
	s.rf.HandleAppendEntries(req, resp)
	raftcore.PrintDebugLog("append entries -> " + resp.String())
	return resp, nil
}

//
// rpc interface
// DoBucketsOperation hanlde bucket data get, delete and insert
//
func (s *ShardKV) DoBucketsOperation(ctx context.Context, req *pb.BucketOperationRequest) (*pb.BucketOperationResponse, error) {
	sOpResp := &pb.BucketOperationResponse{}
	if _, isLeader := s.rf.GetState(); !isLeader {
		return sOpResp, errors.New("ErrorWrongLeader")
	}
	switch req.BucketOpType {
	case pb.BucketOpType_OpGetData:
		{
			s.mu.RLock()
			if s.curConfig.Version < int(req.ConfigVersion) {
				s.mu.RUnlock()
				return sOpResp, errors.New("ErrNotReady")
			}
			bucketDatas := &BucketDatasVo{}
			bucketDatas.Datas = map[int]map[string]string{}
			for _, bucketID := range req.BucketIds {
				sDatas, err := s.stm[int(bucketID)].deepCopy()
				if err != nil {
					s.mu.RUnlock()
					return sOpResp, err
				}
				bucketDatas.Datas[int(bucketID)] = sDatas
			}
			buketDataBytes, _ := json.Marshal(bucketDatas)
			sOpResp.BucketsDatas = buketDataBytes
			sOpResp.ConfigVersion = req.ConfigVersion
			s.mu.RUnlock()
		}
	case pb.BucketOpType_OpDeleteData:
		{
			s.mu.RLock()
			if int64(s.curConfig.Version) > req.ConfigVersion {
				s.mu.RUnlock()
				return sOpResp, nil
			}
			s.mu.RUnlock()
			commandReq := &pb.CommandRequest{}
			bucketOpReqBytes, _ := json.Marshal(req)
			commandReq.Context = bucketOpReqBytes
			commandReq.OpType = pb.OpType_OpDeleteBuckets
			commandReqBytes, _ := json.Marshal(commandReq)
			// async
			_, _, isLeader := s.rf.Propose(commandReqBytes)
			if !isLeader {
				return sOpResp, nil
			}
		}
	case pb.BucketOpType_OpInsertData:
		{
			s.mu.RLock()
			if int64(s.curConfig.Version) > req.ConfigVersion {
				s.mu.RUnlock()
				return sOpResp, nil
			}
			s.mu.RUnlock()
			commandReq := &pb.CommandRequest{}
			bucketOpReqBytes, _ := json.Marshal(req)
			commandReq.Context = bucketOpReqBytes
			commandReq.OpType = pb.OpType_OpInsertBuckets
			commandReqBytes, _ := json.Marshal(commandReq)
			// async
			_, _, isLeader := s.rf.Propose(commandReqBytes)
			if !isLeader {
				return sOpResp, nil
			}
		}
	}
	return sOpResp, nil
}
