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

package configserver

import (
	"context"
	"crypto/rand"
	"math/big"
	"strings"

	"github.com/eraft-io/mit6.824lab2product/common"
	"github.com/eraft-io/mit6.824lab2product/raftcore"
	pb "github.com/eraft-io/mit6.824lab2product/raftpb"
)

type CfgCli struct {
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

func MakeCfgSvrClient(targetId uint64, targetAddrs []string) *CfgCli {

	cfgCli := &CfgCli{
		leaderId:  0,
		clientId:  nrand(),
		commandId: 0,
	}

	for _, addr := range targetAddrs {
		cli := raftcore.MakeRaftClientEnd(addr, targetId)
		cfgCli.endpoints = append(cfgCli.endpoints, cli)
	}

	return cfgCli
}

func (cfgCli *CfgCli) GetRpcClis() []*raftcore.RaftClientEnd {
	return cfgCli.endpoints
}

func (cfgCli *CfgCli) Query(ver int64) *Config {
	confReq := &pb.ConfigRequest{
		OpType:        pb.ConfigOpType_OpQuery,
		ConfigVersion: ver,
	}
	resp := cfgCli.CallDoConfigRpc(confReq)
	cf := &Config{}
	cf.Version = int(resp.Config.ConfigVersion)
	for i := 0; i < common.NBuckets; i++ {
		cf.Buckets[i] = int(resp.Config.Buckets[i])
	}
	cf.Groups = make(map[int][]string)
	for k, v := range resp.Config.Groups {
		serverList := strings.Split(v, ",")
		cf.Groups[int(k)] = serverList
	}
	return cf
}

func (cfgCli *CfgCli) Move(bucket_id, gid int) {
	confReq := &pb.ConfigRequest{
		OpType:   pb.ConfigOpType_OpMove,
		BucketId: int64(bucket_id),
		Gid:      int64(gid),
	}
	resp := cfgCli.CallDoConfigRpc(confReq)
	raftcore.PrintDebugLog("Move bucket resp ok" + resp.ErrMsg)
}

func (cfgCli *CfgCli) Join(servers map[int64]string) {
	confReq := &pb.ConfigRequest{
		OpType:  pb.ConfigOpType_OpJoin,
		Servers: servers,
	}
	resp := cfgCli.CallDoConfigRpc(confReq)
	raftcore.PrintDebugLog("Join cfg resp ok" + resp.ErrMsg)
}

func (cfgCli *CfgCli) Leave(gids []int64) {
	confReq := &pb.ConfigRequest{
		OpType: pb.ConfigOpType_OpLeave,
		Gids:   gids,
	}
	resp := cfgCli.CallDoConfigRpc(confReq)
	raftcore.PrintDebugLog("Leave cfg resp ok" + resp.ErrMsg)
}

func (cfgCli *CfgCli) CallDoConfigRpc(req *pb.ConfigRequest) *pb.ConfigResponse {
	var err error
	confResp := &pb.ConfigResponse{}
	for _, end := range cfgCli.endpoints {
		confResp, err = (*end.GetRaftServiceCli()).DoConfig(context.Background(), req)
		if err != nil {
			raftcore.PrintDebugLog("a node in cluster is down, try next")
			continue
		}
		switch confResp.ErrCode {
		case common.ErrCodeNoErr:
			cfgCli.commandId++
			// raftcore.PrintDebugLog("ErrCodeNoErr")
			return confResp
		case common.ErrCodeWrongLeader:
			confResp, err := (*cfgCli.endpoints[confResp.LeaderId].GetRaftServiceCli()).DoConfig(context.Background(), req)
			if err != nil {
				raftcore.PrintDebugLog("a node in cluster is down : " + err.Error())
				continue
			}
			raftcore.PrintDebugLog("error send to wrong leader, try next node")
			if confResp.ErrCode == common.ErrCodeNoErr {
				cfgCli.commandId++
				return confResp
			}
			return confResp
		}
	}
	return confResp
}
