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

package blockserver

import (
	"github.com/eraft-io/eraft/pkg/log"
	pb "github.com/eraft-io/eraft/pkg/protocol"
	"google.golang.org/grpc"
)

type FileBlockServiceClientEnd struct {
	blockServiceCli *pb.FileBlockServiceClient
	id              uint64
	addr            string
	conns           []*grpc.ClientConn
}

func MakeBlockServerClientEnd(addr string, id uint64) *FileBlockServiceClientEnd {
	conn, err := grpc.Dial(addr, grpc.WithInsecure())
	if err != nil {
		log.MainLogger.Error().Msgf("make connect block server %s err %s", addr, err.Error())
	}
	conns := []*grpc.ClientConn{}
	conns = append(conns, conn)
	rpcClient := pb.NewFileBlockServiceClient(conn)
	return &FileBlockServiceClientEnd{
		id:              id,
		addr:            addr,
		conns:           conns,
		blockServiceCli: &rpcClient,
	}
}

func (blockCliEnd *FileBlockServiceClientEnd) GetFileBlockServiceCli() *pb.FileBlockServiceClient {
	return blockCliEnd.blockServiceCli
}

func (blockCliEnd *FileBlockServiceClientEnd) CloseAllConn() {
	for _, conn := range blockCliEnd.conns {
		conn.Close()
	}
}
