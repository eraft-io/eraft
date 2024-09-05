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
	"bytes"
	"context"
	"encoding/gob"
	"encoding/json"
	"errors"
	"maps"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"github.com/eraft-io/eraft/common"
	"github.com/eraft-io/eraft/logger"
	"github.com/eraft-io/eraft/metaserver"
	pb "github.com/eraft-io/eraft/raftpb"
	storage_eng "github.com/eraft-io/eraft/storage"

	"github.com/eraft-io/eraft/raftcore"
)

type ShardKV struct {
	mu      sync.RWMutex
	dead    int32
	rf      *raftcore.Raft
	applyCh chan *pb.ApplyMsg
	gid_    int
	cvCli   *metaserver.MetaSvrCli

	lastApplied int
	lastConfig  metaserver.Config
	curConfig   metaserver.Config

	stm map[int]*Bucket

	dbEng storage_eng.KvStore

	notifyChans map[int]chan *pb.CommandResponse

	stopApplyCh chan interface{}

	pb.UnimplementedRaftServiceServer
}

type MemSnapshotDB struct {
	KV map[string]string
}

// MakeShardKVServer make a new shard kv server
// peerMaps: init peer map in the raft group
// nodeId: the peer's nodeId in the raft group
// gid: the node's raft group id
// configServerAddr: config server addr (leader addr, need to optimized into config server peer map)
func MakeShardKVServer(peerMaps map[int]string, nodeId int64, gid int, configServerAddrs string) *ShardKV {
	client_ends := []*raftcore.RaftPeerNode{}
	for id, addr := range peerMaps {
		new_end := raftcore.MakeRaftPeerNode(addr, uint64(id))
		client_ends = append(client_ends, new_end)
	}
	new_apply_ch := make(chan *pb.ApplyMsg)

	log_db_eng := storage_eng.EngineFactory("leveldb", "./data/log/datanode_group_"+strconv.Itoa(gid)+"_nodeid_"+strconv.Itoa(int(nodeId)))
	new_rf := raftcore.MakeRaft(client_ends, nodeId, log_db_eng, new_apply_ch, 50, 150)
	newdb_eng := storage_eng.EngineFactory("leveldb", "./data/db/datanode_group_"+strconv.Itoa(gid)+"_nodeid_"+strconv.Itoa(int(nodeId)))

	shard_kv := &ShardKV{
		dead:        0,
		rf:          new_rf,
		applyCh:     new_apply_ch,
		gid_:        gid,
		cvCli:       metaserver.MakeMetaSvrClient(common.UN_UNSED_TID, strings.Split(configServerAddrs, ",")),
		lastApplied: 0,
		curConfig:   metaserver.DefaultConfig(),
		lastConfig:  metaserver.DefaultConfig(),
		stm:         make(map[int]*Bucket),
		dbEng:       newdb_eng,
		notifyChans: map[int]chan *pb.CommandResponse{},
	}

	shard_kv.initStm(shard_kv.dbEng)

	shard_kv.curConfig = *shard_kv.cvCli.Query(-1)
	shard_kv.lastConfig = *shard_kv.cvCli.Query(-1)

	shard_kv.stopApplyCh = make(chan interface{})

	// start applier
	go shard_kv.ApplingToStm(shard_kv.stopApplyCh)

	go shard_kv.ConfigAction()

	return shard_kv
}

// CloseApply close the stopApplyCh to stop commit entries apply
func (s *ShardKV) CloseApply() {
	close(s.stopApplyCh)
}

func (s *ShardKV) GetRf() *raftcore.Raft {
	return s.rf
}

// ConfigAction  sync the config action from configserver
func (s *ShardKV) ConfigAction() {
	for !s.IsKilled() {
		if _, isLeader := s.rf.GetState(); isLeader {
			logger.ELogger().Sugar().Debugf("timeout into config action")

			s.mu.RLock()
			can_perform_next_conf := true
			for _, bucket := range s.stm {
				if bucket.Status != Runing {
					can_perform_next_conf = false
					logger.ELogger().Sugar().Errorf("cano't perform next conf")
					break
				}
			}
			if can_perform_next_conf {
				logger.ELogger().Sugar().Debug("can perform next conf")
			}
			cur_conf_version := s.curConfig.Version
			s.mu.RUnlock()
			if can_perform_next_conf {
				next_config := s.cvCli.Query(int64(cur_conf_version) + 1)
				if next_config == nil {
					continue
				}
				next_cf_bytes, _ := json.Marshal(next_config)
				cur_cf_bytes, _ := json.Marshal(s.curConfig)
				logger.ELogger().Sugar().Debugf("next config %s ", string(next_cf_bytes))
				logger.ELogger().Sugar().Debugf("cur config %s ", string(cur_cf_bytes))
				if next_config.Version == cur_conf_version+1 {
					req := &pb.CommandRequest{}
					next_cf_bytes, _ := json.Marshal(next_config)
					logger.ELogger().Sugar().Debugf("can perform next conf %s ", string(next_cf_bytes))
					req.Context = next_cf_bytes
					req.OpType = pb.OpType_OpConfigChange
					req_bytes, _ := json.Marshal(req)
					idx, _, isLeader := s.rf.Propose(req_bytes)
					if !isLeader {
						return
					}
					s.mu.Lock()
					ch := s.getNotifyChan(idx)
					s.mu.Unlock()

					cmd_resp := &pb.CommandResponse{}

					select {
					case res := <-ch:
						cmd_resp.Value = res.Value
					case <-time.After(metaserver.ExecTimeout):
					}

					logger.ELogger().Sugar().Debug("propose config change ok")

					go func() {
						s.mu.Lock()
						delete(s.notifyChans, idx)
						s.mu.Unlock()
					}()
				}
			}
		}
		time.Sleep(time.Second * 1)
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

// DoCommand do client put get command
func (s *ShardKV) DoCommand(ctx context.Context, req *pb.CommandRequest) (*pb.CommandResponse, error) {

	cmd_resp := &pb.CommandResponse{}

	if !s.CanServe(common.Key2BucketID(req.Key)) {
		cmd_resp.ErrCode = common.ErrCodeWrongGroup
		return cmd_resp, nil
	}
	req_bytes, err := json.Marshal(req)
	if err != nil {
		return nil, err
	}
	// propose to raft
	idx, _, isLeader := s.rf.Propose(req_bytes)
	if !isLeader {
		cmd_resp.ErrCode = common.ErrCodeWrongLeader
		cmd_resp.LeaderId = s.GetRf().GetLeaderId()
		return cmd_resp, nil
	}

	s.mu.Lock()
	ch := s.getNotifyChan(idx)
	s.mu.Unlock()

	select {
	case res := <-ch:
		if res != nil {
			cmd_resp.ErrCode = common.ErrCodeNoErr
			cmd_resp.Value = res.Value
		}
	case <-time.After(metaserver.ExecTimeout):
		return cmd_resp, errors.New("ExecTimeout")
	}

	go func() {
		s.mu.Lock()
		delete(s.notifyChans, idx)
		s.mu.Unlock()
	}()

	return cmd_resp, nil
}

// ApplingToStm  apply the commit operation to state machine
func (s *ShardKV) ApplingToStm(done <-chan interface{}) {
	for !s.IsKilled() {
		select {
		case <-done:
			return
		case appliedMsg := <-s.applyCh:
			logger.ELogger().Sugar().Debugf("appling msg %s", appliedMsg.String())

			if appliedMsg.SnapshotValid {
				s.mu.Lock()
				if s.rf.CondInstallSnapshot(int(appliedMsg.SnapshotTerm), int(appliedMsg.SnapshotIndex)) {
					s.restoreSnapshot(appliedMsg.Snapshot)
					s.lastApplied = int(appliedMsg.SnapshotIndex)
				}
				s.mu.Unlock()
				return
			}

			req := &pb.CommandRequest{}
			if err := json.Unmarshal(appliedMsg.Command, req); err != nil {
				logger.ELogger().Sugar().Errorf("Unmarshal CommandRequest err", err.Error())
				continue
			}
			s.lastApplied = int(appliedMsg.CommandIndex)
			logger.ELogger().Sugar().Debugf("shard_kvserver last applied %d", s.lastApplied)

			cmd_resp := &pb.CommandResponse{}
			value := ""
			var err error
			switch req.OpType {
			// Normal Op
			case pb.OpType_OpPut:
				bucket_id := common.Key2BucketID(req.Key)
				if s.CanServe(bucket_id) {
					logger.ELogger().Sugar().Debug("WRITE put " + req.Key + " value " + req.Value + " to bucket " + strconv.Itoa(bucket_id))
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
					logger.ELogger().Sugar().Debug("get " + req.Key + " value " + value + " from bucket " + strconv.Itoa(bucket_id))
				}
				cmd_resp.Value = value
			case pb.OpType_OpConfigChange:
				next_config := &metaserver.Config{}
				json.Unmarshal(req.Context, next_config)
				if next_config.Version == s.curConfig.Version+1 {
					for i := 0; i < common.NBuckets; i++ {
						if s.curConfig.Buckets[i] != s.gid_ && next_config.Buckets[i] == s.gid_ {
							gid := s.curConfig.Buckets[i]
							if gid != 0 {
								s.stm[i].Status = Runing
							}
						}
						if s.curConfig.Buckets[i] == s.gid_ && next_config.Buckets[i] != s.gid_ {
							gid := next_config.Buckets[i]
							if gid != 0 {
								s.stm[i].Status = Stopped
							}
						}
					}
					s.lastConfig = s.curConfig
					s.curConfig = *next_config
					cf_bytes, _ := json.Marshal(s.curConfig)
					logger.ELogger().Sugar().Debugf("applied config to server %s ", string(cf_bytes))
				}
			case pb.OpType_OpDeleteBuckets:
				bucket_op_reqs := &pb.BucketOperationRequest{}
				json.Unmarshal(req.Context, bucket_op_reqs)
				for _, bid := range bucket_op_reqs.BucketIds {
					s.stm[int(bid)].deleteBucketData()
					logger.ELogger().Sugar().Debugf("del buckets data list %d", strconv.Itoa(int(bid)))
				}
			case pb.OpType_OpInsertBuckets:
				bucket_op_reqs := &pb.BucketOperationRequest{}
				json.Unmarshal(req.Context, bucket_op_reqs)
				bucketDatas := &BucketDatasVo{}
				json.Unmarshal(bucket_op_reqs.BucketsDatas, bucketDatas)
				for bucket_id, kvs := range bucketDatas.Datas {
					s.stm[bucket_id] = NewBucket(s.dbEng, bucket_id)
					for k, v := range kvs {
						s.stm[bucket_id].Put(k, v)
						logger.ELogger().Sugar().Debug("insert kv data to buckets k -> " + k + " v-> " + v)
					}
				}
			}
			if err != nil {
				raftcore.PrintDebugLog(err.Error())
			}

			ch := s.getNotifyChan(int(appliedMsg.CommandIndex))
			ch <- cmd_resp

			if _, isLeader := s.rf.GetState(); isLeader && s.GetRf().LogCount() > 500 {
				s.mu.Lock()
				s.takeSnapshot(uint64(appliedMsg.CommandIndex))
				s.mu.Unlock()
			}
		}
	}
}

// init the status machine
func (s *ShardKV) initStm(eng storage_eng.KvStore) {
	for i := 0; i < common.NBuckets; i++ {
		if _, ok := s.stm[i]; !ok {
			s.stm[i] = NewBucket(eng, i)
		}
	}
}

// takeSnapshot
func (s *ShardKV) takeSnapshot(index uint64) {
	var bytesState bytes.Buffer
	enc := gob.NewEncoder(&bytesState)
	memSnapshotDB := MemSnapshotDB{}
	memSnapshotDB.KV = map[string]string{}
	for i := 0; i < common.NBuckets; i++ {
		if s.CanServe(i) {
			kvs, err := s.stm[i].deepCopy(true)
			if err != nil {
				logger.ELogger().Sugar().Errorf(err.Error())
			}
			maps.Copy(memSnapshotDB.KV, kvs)
		}
	}
	enc.Encode(memSnapshotDB)
	s.GetRf().Snapshot(index, bytesState.Bytes())
}

// restoreSnapshot
func (s *ShardKV) restoreSnapshot(snapData []byte) {
	if snapData == nil {
		return
	}
	buf := bytes.NewBuffer(snapData)
	data := gob.NewDecoder(buf)
	var memSnapshotDB MemSnapshotDB
	if data.Decode(&memSnapshotDB) != nil {
		logger.ELogger().Sugar().Error("decode memsnapshot error")
	}
	for k, v := range memSnapshotDB.KV {
		bucketID := common.Key2BucketID(k)
		if s.CanServe(bucketID) {
			s.stm[bucketID].Put(k, v)
		}
	}
}

// rpc interface
func (s *ShardKV) RequestVote(ctx context.Context, req *pb.RequestVoteRequest) (*pb.RequestVoteResponse, error) {
	resp := &pb.RequestVoteResponse{}
	logger.ELogger().Sugar().Debugf("handle request vote %s ", req.String())

	s.rf.HandleRequestVote(req, resp)
	logger.ELogger().Sugar().Debugf("send request vote resp %s ", resp.String())

	return resp, nil
}

// rpc interface
func (s *ShardKV) AppendEntries(ctx context.Context, req *pb.AppendEntriesRequest) (*pb.AppendEntriesResponse, error) {
	resp := &pb.AppendEntriesResponse{}
	logger.ELogger().Sugar().Debugf("handle append entry %s ", req.String())

	s.rf.HandleAppendEntries(req, resp)
	logger.ELogger().Sugar().Debugf("append entries %s ", resp.String())
	return resp, nil
}

// snapshot rpc interface
func (s *ShardKV) Snapshot(ctx context.Context, req *pb.InstallSnapshotRequest) (*pb.InstallSnapshotResponse, error) {
	resp := &pb.InstallSnapshotResponse{}
	logger.ELogger().Sugar().Debugf("handle snapshot req %s ", req.String())

	s.rf.HandleInstallSnapshot(req, resp)
	logger.ELogger().Sugar().Debugf("handle snapshot resp %s ", resp.String())

	return resp, nil
}

// rpc interface
// DoBucketsOperation hanlde bucket data get, delete and insert
func (s *ShardKV) DoBucketsOperation(ctx context.Context, req *pb.BucketOperationRequest) (*pb.BucketOperationResponse, error) {
	op_resp := &pb.BucketOperationResponse{}
	if _, isLeader := s.rf.GetState(); !isLeader {
		return op_resp, errors.New("ErrorWrongLeader")
	}
	switch req.BucketOpType {
	case pb.BucketOpType_OpGetData:
		{
			s.mu.RLock()
			if s.curConfig.Version < int(req.ConfigVersion) {
				s.mu.RUnlock()
				return op_resp, errors.New("ErrNotReady")
			}
			bucket_datas := &BucketDatasVo{}
			bucket_datas.Datas = map[int]map[string]string{}
			for _, bucketID := range req.BucketIds {
				sDatas, err := s.stm[int(bucketID)].deepCopy(false)
				if err != nil {
					s.mu.RUnlock()
					return op_resp, err
				}
				bucket_datas.Datas[int(bucketID)] = sDatas
			}
			buket_data_bytes, _ := json.Marshal(bucket_datas)
			op_resp.BucketsDatas = buket_data_bytes
			op_resp.ConfigVersion = req.ConfigVersion
			s.mu.RUnlock()
		}
	case pb.BucketOpType_OpDeleteData:
		{
			s.mu.RLock()
			if int64(s.curConfig.Version) > req.ConfigVersion {
				s.mu.RUnlock()
				return op_resp, nil
			}
			s.mu.RUnlock()
			command_req := &pb.CommandRequest{}
			bucket_op_req_bytes, _ := json.Marshal(req)
			command_req.Context = bucket_op_req_bytes
			command_req.OpType = pb.OpType_OpDeleteBuckets
			command_req_bytes, _ := json.Marshal(command_req)
			// async
			_, _, isLeader := s.rf.Propose(command_req_bytes)
			if !isLeader {
				return op_resp, nil
			}
		}
	case pb.BucketOpType_OpInsertData:
		{
			s.mu.RLock()
			if int64(s.curConfig.Version) > req.ConfigVersion {
				s.mu.RUnlock()
				return op_resp, nil
			}
			s.mu.RUnlock()
			command_req := &pb.CommandRequest{}
			bucket_op_req_bytes, _ := json.Marshal(req)
			command_req.Context = bucket_op_req_bytes
			command_req.OpType = pb.OpType_OpInsertBuckets
			command_req_bytes, _ := json.Marshal(command_req)
			// async
			_, _, isLeader := s.rf.Propose(command_req_bytes)
			if !isLeader {
				return op_resp, nil
			}
		}
	}
	return op_resp, nil
}
