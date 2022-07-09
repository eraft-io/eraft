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

type BlockServerClientEnd struct {
	blockServiceCli *pb.FileBlockServiceClient
}

func MakeBlockServerClient(addr string) *BlockServerClientEnd {
	conn, err := grpc.Dial(addr, grpc.WithInsecure())
	if err != nil {
		log.MainLogger.Error().Msgf("make connect block server %s err %s", addr, err.Error())
	}
	blockSvrCli := pb.NewFileBlockServiceClient(conn)
	return &BlockServerClientEnd{
		blockServiceCli: &blockSvrCli,
	}
}

func (b *BlockServerClientEnd) GetBlockSvrCli() pb.FileBlockServiceClient {
	return *b.blockServiceCli
}
