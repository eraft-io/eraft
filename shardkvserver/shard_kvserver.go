// //
// // MIT License

// // Copyright (c) 2022 eraft dev group

// // Permission is hereby granted, free of charge, to any person obtaining a copy
// // of this software and associated documentation files (the "Software"), to deal
// // in the Software without restriction, including without limitation the rights
// // to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// // copies of the Software, and to permit persons to whom the Software is
// // furnished to do so, subject to the following conditions:

// // The above copyright notice and this permission notice shall be included in
// // all copies or substantial portions of the Software.

// // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// // IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// // FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// // AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// // LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// // OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// // SOFTWARE.
// //

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
	"github.com/eraft-io/eraft/storage"

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

	stm             map[int]*Bucket
	dbEng           storage.KvStore
	notifyChs       map[int]chan *pb.CommandResponse
	stopApplyCh     chan interface{}
	lastSnapShotIdx int

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
	clientEnds := []*raftcore.RaftClientEnd{}
	for id, addr := range peerMaps {
		newEnd := raftcore.MakeRaftClientEnd(addr, uint64(id))
		clientEnds = append(clientEnds, newEnd)
	}
	newApplyCh := make(chan *pb.ApplyMsg)

	logDBEng := storage.EngineFactory("leveldb", "./data/log/datanode_group_"+strconv.Itoa(gid)+"_nodeid_"+strconv.Itoa(int(nodeId)))
	newRf := raftcore.MakeRaft(clientEnds, int(nodeId), logDBEng, newApplyCh, 50, 150)
	newDBEng := storage.EngineFactory("leveldb", "./data/db/datanode_group_"+strconv.Itoa(gid)+"_nodeid_"+strconv.Itoa(int(nodeId)))

	shardKv := &ShardKV{
		dead:            0,
		rf:              newRf,
		applyCh:         newApplyCh,
		gid_:            gid,
		cvCli:           metaserver.MakeMetaSvrClient(common.UnUsedTid, strings.Split(configServerAddrs, ",")),
		lastApplied:     0,
		curConfig:       metaserver.DefaultConfig(),
		lastConfig:      metaserver.DefaultConfig(),
		stm:             make(map[int]*Bucket),
		dbEng:           newDBEng,
		lastSnapShotIdx: 0,
		notifyChs:       map[int]chan *pb.CommandResponse{},
	}

	shardKv.initStm(shardKv.dbEng)

	shardKv.curConfig = *shardKv.cvCli.Query(-1)
	shardKv.lastConfig = *shardKv.cvCli.Query(-1)

	shardKv.stopApplyCh = make(chan interface{})
	shardKv.restoreSnapshot(newRf.ReadSnapshot())
	// start applier
	go shardKv.ApplingToStm(shardKv.stopApplyCh)

	go shardKv.ConfigAction()

	return shardKv
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

			canPerformNextConf := true
			s.mu.RLock()
			for _, bucket := range s.stm {
				if bucket.Status != Running {
					canPerformNextConf = false
					logger.ELogger().Sugar().Errorf("cano't perform next conf")
					break
				}
			}
			curConfVersion := s.curConfig.Version
			s.mu.RUnlock()
			if canPerformNextConf {
				logger.ELogger().Sugar().Debug("can perform next conf")
				nextConfig := s.cvCli.Query(int64(curConfVersion) + 1)
				if nextConfig == nil {
					continue
				}
				nextCfBytes, _ := json.Marshal(nextConfig)
				curCfBytes, _ := json.Marshal(s.curConfig)
				logger.ELogger().Sugar().Debugf("next config %s ", string(nextCfBytes))
				logger.ELogger().Sugar().Debugf("cur config %s ", string(curCfBytes))
				if nextConfig.Version == curConfVersion+1 {
					req := &pb.CommandRequest{}
					nextCfBytes, _ := json.Marshal(nextConfig)
					logger.ELogger().Sugar().Debugf("can perform next conf %s ", string(nextCfBytes))
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
					case <-time.After(metaserver.ExecTimeout):
					}

					logger.ELogger().Sugar().Debug("propose config change ok")

					go func() {
						s.mu.Lock()
						delete(s.notifyChs, idx)
						s.mu.Unlock()
					}()
				}
			}
		}
		time.Sleep(time.Second * 1)
	}
}

// getBucketIDsByStatus get bucket ids by status
func (s *ShardKV) getBucketIDsByStatus(status buketStatus) map[int][]int {
	g2bkids := make(map[int][]int)
	for i, bucket := range s.stm {
		if bucket.Status == status {
			gid := s.lastConfig.Buckets[i]
			if gid != 0 {
				if _, ok := g2bkids[gid]; !ok {
					g2bkids[gid] = make([]int, 0)
				}
				g2bkids[gid] = append(g2bkids[gid], i)
			}
		}
	}
	return g2bkids
}

func (s *ShardKV) CanServe(bucketId int) bool {
	return s.curConfig.Buckets[bucketId] == s.gid_ && (s.stm[bucketId].Status == Running)
}

func (s *ShardKV) getNotifyChan(index int) chan *pb.CommandResponse {
	if _, ok := s.notifyChs[index]; !ok {
		s.notifyChs[index] = make(chan *pb.CommandResponse, 1)
	}
	return s.notifyChs[index]
}

func (s *ShardKV) IsKilled() bool {
	return atomic.LoadInt32(&s.dead) == 1
}

// DoCommand do client put get command
func (s *ShardKV) DoCommand(ctx context.Context, req *pb.CommandRequest) (*pb.CommandResponse, error) {

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
	case <-time.After(metaserver.ExecTimeout):
		return cmdResp, errors.New("ExecTimeout")
	}

	go func() {
		s.mu.Lock()
		delete(s.notifyChs, idx)
		s.mu.Unlock()
	}()

	return cmdResp, nil
}

// ApplingToStm  apply the commit operation to state machine
func (s *ShardKV) ApplingToStm(done <-chan any) {
	for !s.IsKilled() {
		select {
		case <-done:
			return
		case appliedMsg := <-s.applyCh:
			logger.ELogger().Sugar().Debugf("nodeid %v, groupid %v, appling msg %s", s.rf.GetMyId(), s.gid_, appliedMsg.String())
			if appliedMsg.CommandValid {
				s.mu.Lock()

				// decode command
				req := &pb.CommandRequest{}
				if err := json.Unmarshal(appliedMsg.Command, req); err != nil {
					logger.ELogger().Sugar().Error("Unmarshal CommandRequest err", err.Error())
					s.mu.Unlock()
					continue
				}

				// outdate checked
				if appliedMsg.CommandIndex <= int64(s.lastApplied) {
					s.mu.Unlock()
					continue
				}

				s.lastApplied = int(appliedMsg.CommandIndex)
				logger.ELogger().Sugar().Debugf("shard_kvserver last applied %d", s.lastApplied)

				cmdResp := &pb.CommandResponse{}
				switch req.OpType {
				// normal operations
				case pb.OpType_OpPut:
					bucketID := common.Key2BucketID(req.Key)
					if s.CanServe(bucketID) {
						logger.ELogger().Sugar().Debug("put " + req.Key + " value " + req.Value + " to bucket " + strconv.Itoa(bucketID))
						s.stm[bucketID].Put(req.Key, req.Value)
					}
				case pb.OpType_OpAppend:
					bucketID := common.Key2BucketID(req.Key)
					if s.CanServe(bucketID) {
						s.stm[bucketID].Append(req.Key, req.Value)
					}
				case pb.OpType_OpGet:
					bucketID := common.Key2BucketID(req.Key)
					if s.CanServe(bucketID) {
						value, err_ := s.stm[bucketID].Get(req.Key)
						if err_ != nil {
							logger.ELogger().Sugar().Errorf(err_.Error())
						}
						logger.ELogger().Sugar().Debug("get " + req.Key + " value " + value + " from bucket " + strconv.Itoa(bucketID))
						cmdResp.Value = value
					}
				// config change operations
				case pb.OpType_OpConfigChange:
					nextConfig := &metaserver.Config{}
					json.Unmarshal(req.Context, nextConfig)
					if nextConfig.Version == s.curConfig.Version+1 {
						for i := 0; i < common.NBuckets; i++ {
							// bucket move to my group
							if s.curConfig.Buckets[i] != s.gid_ && nextConfig.Buckets[i] == s.gid_ {
								gid := s.curConfig.Buckets[i]
								if gid != 0 {
									s.stm[i].Status = Running
								}
							}
							// bucket move to other group
							if s.curConfig.Buckets[i] == s.gid_ && nextConfig.Buckets[i] != s.gid_ {
								gid := nextConfig.Buckets[i]
								if gid != 0 {
									s.stm[i].Status = Running
								}
							}
						}
						s.lastConfig = s.curConfig
						s.curConfig = *nextConfig
						cfBytes, _ := json.Marshal(s.curConfig)
						logger.ELogger().Sugar().Debugf("applied config to server %s ", string(cfBytes))
					}
				case pb.OpType_OpDeleteBuckets:
					bucketOpReqs := &pb.BucketOperationRequest{}
					json.Unmarshal(req.Context, bucketOpReqs)
					for _, bid := range bucketOpReqs.BucketIds {
						s.stm[int(bid)].deleteBucketData()
						logger.ELogger().Sugar().Debugf("del buckets data list %d", bid)
					}
				case pb.OpType_OpInsertBuckets:
					bucketOpReqs := &pb.BucketOperationRequest{}
					json.Unmarshal(req.Context, bucketOpReqs)
					bucketDatas := &BucketDatasVo{}
					json.Unmarshal(bucketOpReqs.BucketsDatas, bucketDatas)
					for bucketId, kvs := range bucketDatas.Datas {
						s.stm[bucketId] = NewBucket(s.dbEng, bucketId)
						for k, v := range kvs {
							s.stm[bucketId].Put(k, v)
							logger.ELogger().Sugar().Debug("insert kv data to buckets k -> " + k + " v-> " + v)
						}
					}
				}

				if _, isLeader := s.rf.GetState(); isLeader {
					ch := s.getNotifyChan(int(appliedMsg.CommandIndex))
					ch <- cmdResp
				}

				if s.GetRf().GetLogCount() > 50 {
					s.takeSnapshot(uint64(appliedMsg.CommandIndex))
				}

				s.mu.Unlock()

			} else if appliedMsg.SnapshotValid {
				s.mu.Lock()
				if s.rf.CondInstallSnapshot(int(appliedMsg.SnapshotTerm), int(appliedMsg.SnapshotIndex), appliedMsg.Snapshot) {
					s.restoreSnapshot(appliedMsg.Snapshot)
					s.lastApplied = int(appliedMsg.SnapshotIndex)
				}
				s.mu.Unlock()
				return
			} else {
				panic("appliedMsg is not valid")
			}
		}
	}
}

// init the status machine
func (s *ShardKV) initStm(eng storage.KvStore) {
	for i := 0; i < common.NBuckets; i++ {
		if _, ok := s.stm[i]; !ok {
			s.stm[i] = NewBucket(eng, i)
		}
	}
}

// takeSnapshot
func (s *ShardKV) takeSnapshot(index uint64) {
	logger.ELogger().Sugar().Infof("start take snapshot at % d", index)
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
	s.GetRf().Snapshot(int(index), bytesState.Bytes())
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
	logger.ELogger().Sugar().Debugf("node gid %d, raft node id %d, handle append entry %s ", s.gid_, s.rf.GetMyId(), req.String())

	s.rf.HandleAppendEntries(req, resp)
	logger.ELogger().Sugar().Debugf("node gid %d, raft node id %d, append entries %s ", s.gid_, s.rf.GetMyId(), resp.String())
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

// DoBucketsOperation rpc interface
// handle bucket data get, delete and insert
func (s *ShardKV) DoBucketsOperation(ctx context.Context, req *pb.BucketOperationRequest) (*pb.BucketOperationResponse, error) {
	opResp := &pb.BucketOperationResponse{}
	if _, isLeader := s.rf.GetState(); !isLeader {
		return opResp, errors.New("ErrorWrongLeader")
	}
	switch req.BucketOpType {
	case pb.BucketOpType_OpGetData:
		{
			s.mu.RLock()
			if s.curConfig.Version < int(req.ConfigVersion) {
				s.mu.RUnlock()
				return opResp, errors.New("ErrNotReady")
			}
			bucketDatas := &BucketDatasVo{}
			bucketDatas.Datas = map[int]map[string]string{}
			for _, bucketID := range req.BucketIds {
				sDatas, err := s.stm[int(bucketID)].deepCopy(false)
				if err != nil {
					s.mu.RUnlock()
					return opResp, err
				}
				bucketDatas.Datas[int(bucketID)] = sDatas
			}
			buketDataBytes, _ := json.Marshal(bucketDatas)
			opResp.BucketsDatas = buketDataBytes
			opResp.ConfigVersion = req.ConfigVersion
			s.mu.RUnlock()
		}
	case pb.BucketOpType_OpDeleteData:
		{
			s.mu.RLock()
			if int64(s.curConfig.Version) > req.ConfigVersion {
				s.mu.RUnlock()
				return opResp, nil
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
				return opResp, nil
			}
		}
	case pb.BucketOpType_OpInsertData:
		{
			s.mu.RLock()
			if int64(s.curConfig.Version) > req.ConfigVersion {
				s.mu.RUnlock()
				return opResp, nil
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
				return opResp, nil
			}
		}
	}
	return opResp, nil
}
