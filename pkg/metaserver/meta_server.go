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
	"errors"
	"fmt"
	"strings"
	"sync"
	"time"

	"github.com/eraft-io/eraft/pkg/common"
	"github.com/eraft-io/eraft/pkg/consts"
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
	stm         TopoConfigSTM
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
		stm:         NewTopoConfigSTM(metaStorage),
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

func (s *MetaServer) ServerGroupMeta(ctx context.Context, req *pb.ServerGroupMetaConfigRequest) (*pb.ServerGroupMetaConfigResponse, error) {
	log.MainLogger.Debug().Msgf("handle server group meta req: %s", req.String())
	resp := &pb.ServerGroupMetaConfigResponse{}
	reqByteSeq := EncodeServerGroupMetaRequest(req)
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
		resp.BucketOpRes = res.BucketOpRes
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
			req := DecodeServerGroupMetaRequest(appliedMsg.Command)
			resp := &pb.ServerGroupMetaConfigResponse{}
			switch req.OpType {
			case pb.ConfigServerGroupMetaOpType_OP_SERVER_GROUP_JOIN:
				{
					serverGroups := map[int][]string{}
					for gid, addrs := range req.ServerGroups {
						serverGroups[int(gid)] = strings.Split(addrs, ",")
					}
					err := s.stm.Join(serverGroups)
					if err != nil {
						resp.ErrCode = pb.ErrCode_APPLY_JOIN_TODO_TO_STM_ERR
					}
				}
			case pb.ConfigServerGroupMetaOpType_OP_SERVER_GROUP_LEAVE:
				{
					gids := []int{}
					for _, gid := range req.Gids {
						gids = append(gids, int(gid))
					}
					if err := s.stm.Leave(gids); err != nil {
						resp.ErrCode = pb.ErrCode_APPLY_LEAVE_TODO_TO_STM_ERR
					}
				}
			case pb.ConfigServerGroupMetaOpType_OP_SERVER_GROUP_QUERY:
				{
					topoConf, err := s.stm.Query(int(req.ConfigVersion))
					if err != nil {
						resp.ErrCode = pb.ErrCode_APPLY_QUERY_TOPO_CONF_ERR
					}
					resp.ServerGroupMetas = &pb.ServerGroupMetas{}
					resp.ServerGroupMetas.ConfigVersion = int64(topoConf.Version)
					resp.ServerGroupMetas.ServerGroups = make(map[int64]string)
					for gid, servers := range topoConf.ServerGroups {
						resp.ServerGroupMetas.ServerGroups[int64(gid)] = strings.Join(servers, ",")
					}
				}
			case pb.ConfigServerGroupMetaOpType_OP_OSS_BUCKET_ADD:
				{
					bucketId := common.GenGoogleUUID()
					bucket := &pb.Bucket{
						BucketId:   bucketId,
						BucketName: req.BucketOpReq.BucketName,
					}
					log.MainLogger.Debug().Msgf("add bucket %s with id %s", bucket.BucketName, bucket.BucketId)
					bucketEncodeKey := EncodeBucketKey(bucketId)
					bucketEncodeVal := EncodeBucket(bucket)
					if err := s.metaEng.Put(bucketEncodeKey, bucketEncodeVal); err != nil {
						resp.ErrCode = pb.ErrCode_PUT_BUCKET_TO_ENG_ERR
					}
					resp.ErrCode = pb.ErrCode_NO_ERR
				}
			case pb.ConfigServerGroupMetaOpType_OP_OSS_BUCKET_DEL:
				{
					bucketEncodeKey := EncodeBucketKey(req.BucketOpReq.BucketId)
					if err := s.metaEng.Del(bucketEncodeKey); err != nil {
						resp.ErrCode = pb.ErrCode_DEL_BUCKET_FROM_ENG_ERR
					}
					resp.ErrCode = pb.ErrCode_NO_ERR
				}
			case pb.ConfigServerGroupMetaOpType_OP_OSS_BUCKET_LIST:
				{
					_, vals, err := s.metaEng.GetPrefixRangeKvs(consts.BUCKET_META_PREFIX)
					if err != nil {
						panic(err.Error())
					}
					resp.BucketOpRes = &pb.BucketOpResponse{}
					resp.BucketOpRes.Buckets = make([]*pb.Bucket, 0)
					log.MainLogger.Debug().Msgf("get vals count %d", len(vals))
					for _, v := range vals {
						bucket := DecodeBucket([]byte(v))
						log.MainLogger.Debug().Msgf("decode bucket %v", []byte(v))
						resp.BucketOpRes.Buckets = append(resp.BucketOpRes.Buckets, bucket)
					}
					resp.ErrCode = pb.ErrCode_NO_ERR
				}
			}
			log.MainLogger.Debug().Msgf("apply op to meta server stm: %s", req.String())
			ch := s.getRespNotifyChan(appliedMsg.CommandIndex)
			ch <- resp
		}
	}
}

func (s *MetaServer) FileBlockMeta(ctx context.Context, req *pb.FileBlockMetaConfigRequest) (*pb.FileBlockMetaConfigResponse, error) {
	return nil, nil
}
