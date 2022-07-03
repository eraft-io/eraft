package metaserver

import (
	pb "github.com/eraft-io/eraft/pkg/protocol"
	"google.golang.org/grpc"
)

type MetaServerClientEnd struct {
	conns          []*grpc.ClientConn
	metaServiceCli *pb.MetaServiceClient
}

func (m *MetaServerClientEnd) GetMetaSvrCli() pb.MetaServiceClient {
	return *m.metaServiceCli
}
