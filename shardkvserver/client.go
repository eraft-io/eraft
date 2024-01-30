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
// TODO: this is a debug client version, need to deal with more detail handle
//

package shardkvserver

import (
	"context"
	"crypto/rand"
	"errors"
	"fmt"
	"math/big"
	"strings"

	"github.com/eraft-io/eraft/common"
	"github.com/eraft-io/eraft/logger"
	"github.com/eraft-io/eraft/metaserver"
	"github.com/eraft-io/eraft/raftcore"
	pb "github.com/eraft-io/eraft/raftpb"
)

// a client is defined for the shard_kvserver
type KvClient struct {
	// raft group rpc client
	rpcCli *raftcore.RaftPeerNode

	connectsCache map[string]*raftcore.RaftPeerNode

	// config server group client
	csCli *metaserver.MetaSvrCli
	// current config, got from config server group
	config *metaserver.Config
	// leader id, got from config server group
	leaderId int64
	// the client id, use to identify a client
	clientId int64
	// the command id, use to identify a command
	commandId int64
}

func (cli *KvClient) AddConnToCache(svrAddr string, rpcCli *raftcore.RaftPeerNode) {
	cli.connectsCache[svrAddr] = rpcCli
}

func (cli *KvClient) GetConnFromCache(svrAddr string) *raftcore.RaftPeerNode {
	if conn, ok := cli.connectsCache[svrAddr]; ok {
		return conn
	}
	return nil
}

func (cli *KvClient) CloseRpcCliConn() {
	cli.rpcCli.CloseAllConn()
}

// expose config server group clients to the outside
func (cli *KvClient) GetCsClient() *metaserver.MetaSvrCli {
	return cli.csCli
}

// expose raft group rpc client to the outside
func (cli *KvClient) GetRpcClient() *raftcore.RaftPeerNode {
	return cli.rpcCli
}

// make a random id
func nrand() int64 {
	max := big.NewInt(int64(1) << 62)
	bigx, _ := rand.Int(rand.Reader, max)
	return bigx.Int64()
}

// make a kv cilent
func MakeKvClient(csAddrs string) *KvClient {
	meta_svr_cli := metaserver.MakeMetaSvrClient(common.UN_UNSED_TID, strings.Split(csAddrs, ","))
	kv_cli := &KvClient{
		csCli:     meta_svr_cli,
		rpcCli:    nil,
		leaderId:  0,
		clientId:  nrand(),
		commandId: 0,
	}
	kv_cli.config = kv_cli.csCli.Query(-1)
	return kv_cli
}

// get interface to client, use to get a key's data from the cluster
func (kvCli *KvClient) Get(key string) (string, error) {
	return kvCli.Command(&pb.CommandRequest{
		Key:    key,
		OpType: pb.OpType_OpGet,
	})
}

// put interface to client, use to put key, value data to the cluster
func (kvCli *KvClient) Put(key, value string) error {
	_, err := kvCli.Command(&pb.CommandRequest{
		Key:    key,
		Value:  value,
		OpType: pb.OpType_OpPut,
	})
	return err
}

// GetBucketDatas
// get all the data in a bucket, this is not an efficient approach to data migration
// and needs to be optimized
func (kvCli *KvClient) GetBucketDatas(gid int, bucketIds []int64) string {
	return kvCli.BucketOpCommand(&pb.BucketOperationRequest{
		BucketOpType:  pb.BucketOpType_OpGetData,
		Gid:           int64(gid),
		ConfigVersion: int64(kvCli.config.Version),
		BucketIds:     bucketIds,
	})
}

// DeleteBucketDatas
// delete all the data in a bucket, this is not an efficient approach to data migration
// and needs to be optimized
func (kvCli *KvClient) DeleteBucketDatas(gid int, bucketIds []int64) string {
	return kvCli.BucketOpCommand(&pb.BucketOperationRequest{
		BucketOpType:  pb.BucketOpType_OpDeleteData,
		Gid:           int64(gid),
		ConfigVersion: int64(kvCli.config.Version),
		BucketIds:     bucketIds,
	})
}

// InsertBucketDatas
// insert all the data into a bucket, this is not an efficient approach to data migration
// and needs to be optimized
func (kvCli *KvClient) InsertBucketDatas(gid int, bucketIds []int64, datas []byte) string {
	return kvCli.BucketOpCommand(&pb.BucketOperationRequest{
		BucketOpType:  pb.BucketOpType_OpInsertData,
		BucketsDatas:  datas,
		Gid:           int64(gid),
		BucketIds:     bucketIds,
		ConfigVersion: int64(kvCli.config.Version),
	})
}

// Command
// do user normal command
func (kvCli *KvClient) Command(req *pb.CommandRequest) (string, error) {
	bucket_id := common.Key2BucketID(req.Key)
	gid := kvCli.config.Buckets[bucket_id]
	if gid == 0 {
		return "", errors.New("there is no shard in charge of this bucket, please join the server group before")
	}
	if servers, ok := kvCli.config.Groups[gid]; ok {
		for _, svrAddr := range servers {
			if kvCli.GetConnFromCache(svrAddr) == nil {
				kvCli.rpcCli = raftcore.MakeRaftPeerNode(svrAddr, common.UN_UNSED_TID)
			} else {
				kvCli.rpcCli = kvCli.GetConnFromCache(svrAddr)
			}
			resp, err := (*kvCli.rpcCli.GetRaftServiceCli()).DoCommand(context.Background(), req)
			if err != nil {
				// node down
				logger.ELogger().Sugar().Debugf("there is a node down is cluster, but we can continue with outher node")
				continue
			}
			switch resp.ErrCode {
			case common.ErrCodeNoErr:
				kvCli.commandId++
				return resp.Value, nil
			case common.ErrCodeWrongGroup:
				kvCli.config = kvCli.csCli.Query(-1)
				return "", errors.New("WrongGroup")
			case common.ErrCodeWrongLeader:
				kvCli.rpcCli = raftcore.MakeRaftPeerNode(servers[resp.LeaderId], common.UN_UNSED_TID)
				resp, err := (*kvCli.rpcCli.GetRaftServiceCli()).DoCommand(context.Background(), req)
				if err != nil {
					fmt.Printf("err %s", err.Error())
					panic(err)
				}
				if resp.ErrCode == common.ErrCodeNoErr {
					kvCli.commandId++
					return resp.Value, nil
				}
			default:
				return "", errors.New("unknow code")
			}
		}
	} else {
		return "", errors.New("please join the server group first")
	}
	return "", errors.New("unknow code")
}

// BucketOpCommand
// do user bucket operation command
func (kvCli *KvClient) BucketOpCommand(req *pb.BucketOperationRequest) string {
	for {
		if servers, ok := kvCli.config.Groups[int(req.Gid)]; ok {
			for _, svrAddr := range servers {
				kvCli.rpcCli = raftcore.MakeRaftPeerNode(svrAddr, common.UN_UNSED_TID)
				resp, err := (*kvCli.rpcCli.GetRaftServiceCli()).DoBucketsOperation(context.Background(), req)
				if err == nil {
					if resp != nil {
						return string(resp.BucketsDatas)
					} else {
						return ""
					}
				} else {
					logger.ELogger().Sugar().Errorf("send command to server error", err.Error())
					return ""
				}
			}
		}
	}
}
