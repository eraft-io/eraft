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
	"github.com/eraft-io/eraft/pkg/log"
	pb "github.com/eraft-io/eraft/pkg/protocol"
	"google.golang.org/grpc"
)

type MetaServiceClientEnd struct {
	metaServiceCli *pb.MetaServiceClient
	id             uint64
	addr           string
	conns          []*grpc.ClientConn
}

func (metaSvrEnd *MetaServiceClientEnd) Id() uint64 {
	return metaSvrEnd.id
}

func (metaSvrEnd *MetaServiceClientEnd) GetMetaServiceCli() *pb.MetaServiceClient {
	return metaSvrEnd.metaServiceCli
}

func MakeMetaServiceClientEnd(addr string, id uint64) *MetaServiceClientEnd {
	conn, err := grpc.Dial(addr, grpc.WithInsecure())
	if err != nil {
		log.MainLogger.Error().Msgf("err: ", err.Error())
	}
	conns := []*grpc.ClientConn{}
	conns = append(conns, conn)
	rpcClient := pb.NewMetaServiceClient(conn)
	return &MetaServiceClientEnd{
		id:             id,
		addr:           addr,
		conns:          conns,
		metaServiceCli: &rpcClient,
	}
}

func (metaSvrEnd *MetaServiceClientEnd) CloseAllConn() {
	for _, conn := range metaSvrEnd.conns {
		conn.Close()
	}
}
