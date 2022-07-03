package blockserver

import (
	pb "github.com/eraft-io/eraft/pkg/protocol"
	"google.golang.org/grpc"
)

type BlockServerClientEnd struct {
	conns           []*grpc.ClientConn
	blockServiceCli *pb.FileBlockServiceClient
}

func MakeBlockServerClient(addrs string) *BlockServerClientEnd {
	return nil
}

func (b *BlockServerClientEnd) GetBlockSvrCli() pb.FileBlockServiceClient {
	return *b.blockServiceCli
}
