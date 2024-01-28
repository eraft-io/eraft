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
// TODO: this is a debug client version, need to deal with more detail handle
//

package metaserver

import (
	"context"
	"crypto/rand"
	"math/big"
	"strings"

	"github.com/eraft-io/eraft/common"
	"github.com/eraft-io/eraft/logger"
	"github.com/eraft-io/eraft/raftcore"
	pb "github.com/eraft-io/eraft/raftpb"
)

type MetaSvrCli struct {
	endpoints []*raftcore.RaftClientEnd
	leaderId  int64
	clientId  int64
	commandId int64
}

func nrand() int64 {
	max := big.NewInt(int64(1) << 62)
	bigx, _ := rand.Int(rand.Reader, max)
	return bigx.Int64()
}

func MakeMetaSvrClient(targetId uint64, targetAddrs []string) *MetaSvrCli {

	mate_svr_cli := &MetaSvrCli{
		leaderId:  0,
		clientId:  nrand(),
		commandId: 0,
	}

	for _, addr := range targetAddrs {
		cli := raftcore.MakeRaftClientEnd(addr, targetId)
		mate_svr_cli.endpoints = append(mate_svr_cli.endpoints, cli)
	}

	return mate_svr_cli
}

func (meta_svr_cli *MetaSvrCli) GetRpcClis() []*raftcore.RaftClientEnd {
	return meta_svr_cli.endpoints
}

func (meta_svr_cli *MetaSvrCli) Query(ver int64) *Config {
	conf_req := &pb.ConfigRequest{
		OpType:        pb.ConfigOpType_OpQuery,
		ConfigVersion: ver,
	}
	resp := meta_svr_cli.CallDoConfigRpc(conf_req)
	cf := &Config{}
	if resp != nil {
		cf.Version = int(resp.Config.ConfigVersion)
		for i := 0; i < common.NBuckets; i++ {
			cf.Buckets[i] = int(resp.Config.Buckets[i])
		}
		cf.Groups = make(map[int][]string)
		for k, v := range resp.Config.Groups {
			serverList := strings.Split(v, ",")
			cf.Groups[int(k)] = serverList
		}
	}
	return cf
}

func (meta_svr_cli *MetaSvrCli) Move(bucket_id, gid int) *pb.ConfigResponse {
	conf_req := &pb.ConfigRequest{
		OpType:   pb.ConfigOpType_OpMove,
		BucketId: int64(bucket_id),
		Gid:      int64(gid),
	}
	return meta_svr_cli.CallDoConfigRpc(conf_req)
}

func (meta_svr_cli *MetaSvrCli) Join(servers map[int64]string) *pb.ConfigResponse {
	conf_req := &pb.ConfigRequest{
		OpType:  pb.ConfigOpType_OpJoin,
		Servers: servers,
	}
	return meta_svr_cli.CallDoConfigRpc(conf_req)
}

func (meta_svr_cli *MetaSvrCli) Leave(gids []int64) *pb.ConfigResponse {
	conf_req := &pb.ConfigRequest{
		OpType: pb.ConfigOpType_OpLeave,
		Gids:   gids,
	}
	return meta_svr_cli.CallDoConfigRpc(conf_req)
}

func (meta_svr_cli *MetaSvrCli) CallDoConfigRpc(req *pb.ConfigRequest) *pb.ConfigResponse {
	var err error
	conf_resp := &pb.ConfigResponse{}
	conf_resp.Config = &pb.ServerConfig{}
	for _, end := range meta_svr_cli.endpoints {
		conf_resp, err = (*end.GetRaftServiceCli()).DoConfig(context.Background(), req)
		if err != nil {
			continue
		}
		switch conf_resp.ErrCode {
		case common.ErrCodeNoErr:
			meta_svr_cli.commandId++
			return conf_resp
		case common.ErrCodeWrongLeader:
			conf_resp, err := (*meta_svr_cli.endpoints[conf_resp.LeaderId].GetRaftServiceCli()).DoConfig(context.Background(), req)
			if err != nil {
				logger.ELogger().Sugar().Debugf("a node in cluster is down : ", err.Error())
				continue
			}
			if conf_resp.ErrCode == common.ErrCodeNoErr {
				meta_svr_cli.commandId++
				return conf_resp
			}
			if conf_resp.ErrCode == common.ErrCodeExecTimeout {
				logger.ELogger().Sugar().Debug("exec timeout")
				return conf_resp
			}
			return conf_resp
		}
	}
	return conf_resp
}
