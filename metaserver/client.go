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
	leaderID  int64
	clientID  int64
	commandID int64
}

func nrand() int64 {
	maxi := big.NewInt(int64(1) << 62)
	bigx, _ := rand.Int(rand.Reader, maxi)
	return bigx.Int64()
}

func MakeMetaSvrClient(targetID uint64, targetAddrs []string) *MetaSvrCli {
	mateSvrCli := &MetaSvrCli{
		leaderID:  0,
		clientID:  nrand(),
		commandID: 0,
	}

	for _, addr := range targetAddrs {
		cli := raftcore.MakeRaftClientEnd(addr, targetID)
		mateSvrCli.endpoints = append(mateSvrCli.endpoints, cli)
	}

	return mateSvrCli
}

func (metaSvrCli *MetaSvrCli) GetRpcClis() []*raftcore.RaftClientEnd {
	return metaSvrCli.endpoints
}

func (metaSvrCli *MetaSvrCli) Query(ver int64) *Config {
	confReq := &pb.ConfigRequest{
		OpType:        pb.ConfigOpType_OpQuery,
		ConfigVersion: ver,
	}
	resp := metaSvrCli.CallDoConfigRpc(confReq)
	cf := &Config{}
	if resp != nil && resp.Config != nil {
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

func (metaSvrCli *MetaSvrCli) Move(bucketID, gid int) *pb.ConfigResponse {
	confReq := &pb.ConfigRequest{
		OpType:   pb.ConfigOpType_OpMove,
		BucketId: int64(bucketID),
		Gid:      int64(gid),
	}
	return metaSvrCli.CallDoConfigRpc(confReq)
}

func (metaSvrCli *MetaSvrCli) Join(servers map[int64]string) *pb.ConfigResponse {
	confReq := &pb.ConfigRequest{
		OpType:  pb.ConfigOpType_OpJoin,
		Servers: servers,
	}
	return metaSvrCli.CallDoConfigRpc(confReq)
}

func (metaSvrCli *MetaSvrCli) Leave(gids []int64) *pb.ConfigResponse {
	confReq := &pb.ConfigRequest{
		OpType: pb.ConfigOpType_OpLeave,
		Gids:   gids,
	}
	return metaSvrCli.CallDoConfigRpc(confReq)
}

func (metaSvrCli *MetaSvrCli) CallDoConfigRpc(req *pb.ConfigRequest) *pb.ConfigResponse {
	var err error
	confResp := &pb.ConfigResponse{}
	confResp.Config = &pb.ServerConfig{}
	for _, end := range metaSvrCli.endpoints {
		confResp, err = (*end.GetRaftServiceCli()).DoConfig(context.Background(), req)
		if err != nil {
			continue
		}
		switch confResp.ErrCode {
		case common.ErrCodeNoErr:
			metaSvrCli.commandID++
			return confResp
		case common.ErrCodeWrongLeader:
			confResp, err := (*metaSvrCli.endpoints[confResp.LeaderId].GetRaftServiceCli()).DoConfig(context.Background(), req)
			if err != nil {
				logger.ELogger().Sugar().Debug("a node in cluster is down :", err.Error())
				continue
			}
			if confResp.ErrCode == common.ErrCodeNoErr {
				metaSvrCli.commandID++
				return confResp
			}
			if confResp.ErrCode == common.ErrCodeExecTimeout {
				logger.ELogger().Sugar().Debug("exec timeout")
				return confResp
			}
			return confResp
		}
	}
	return confResp
}
