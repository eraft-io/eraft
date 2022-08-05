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

	"github.com/eraft-io/eraft/pkg/log"
	pb "github.com/eraft-io/eraft/pkg/protocol"
)

type MetaSvrCli struct {
	endpoints []*MetaServiceClientEnd
}

func MakeMetaServerClient(metaServerAddrs []string) *MetaSvrCli {
	metaSvrCli := &MetaSvrCli{}
	for i, addr := range metaServerAddrs {
		endpoint := MakeMetaServiceClientEnd(addr, uint64(i))
		metaSvrCli.endpoints = append(metaSvrCli.endpoints, endpoint)
	}
	return metaSvrCli
}

func (cli *MetaSvrCli) CallServerGroupMeta(req *pb.ServerGroupMetaConfigRequest) *pb.ServerGroupMetaConfigResponse {
	resp := &pb.ServerGroupMetaConfigResponse{}
	resp.ServerGroupMetas = &pb.ServerGroupMetas{}
	var err error
	for _, end := range cli.endpoints {
		resp, err = (*end.GetMetaServiceCli()).ServerGroupMeta(context.Background(), req)
		if err != nil {
			log.MainLogger.Warn().Msgf("a node in cluster is down, try next")
			continue
		}
		switch resp.ErrCode {
		case pb.ErrCode_NO_ERR:
			return resp
		case pb.ErrCode_WRONG_LEADER_ERR:
			log.MainLogger.Debug().Msgf("find leader with id %d", resp.LeaderId)
			resp, err := (*cli.endpoints[resp.LeaderId].GetMetaServiceCli()).ServerGroupMeta(context.Background(), req)
			if err != nil {
				log.MainLogger.Error().Msgf("a node in cluster is down : " + err.Error())
				continue
			}
			if resp.ErrCode == pb.ErrCode_RPC_CALL_TIMEOUT_ERR {
				log.MainLogger.Error().Msgf("exec timeout")
			}
			return resp
		}
	}
	return resp
}
