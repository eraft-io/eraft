package main

import (
	"flag"
	"fmt"
	"net"
	"strings"

	"github.com/eraft-io/eraft/kvraft"
	"github.com/eraft-io/eraft/kvraftpb"
	"github.com/eraft-io/eraft/raft"
	"github.com/eraft-io/eraft/raftpb"
	"google.golang.org/grpc"
)

func main() {
	id := flag.Int("id", 0, "node id")
	addrs := flag.String("addrs", "localhost:5001,localhost:5002,localhost:5003", "comma separated addresses")
	dbPath := flag.String("db", "kvserver-data", "path to leveldb")
	flag.Parse()

	addrList := strings.Split(*addrs, ",")
	peers := make([]raft.RaftPeer, len(addrList))
	for i, addr := range addrList {
		if i == *id {
			continue
		}
		peers[i] = raft.NewRaftgRPCClient(addr)
	}

	persister := raft.MakeFilePersister(fmt.Sprintf("raft-state-%d", *id))
	kvServer := kvraft.StartKVServer(peers, *id, persister, -1, *dbPath)

	// Start gRPC server
	lis, err := net.Listen("tcp", addrList[*id])
	if err != nil {
		panic(err)
	}
	s := grpc.NewServer()
	raftpb.RegisterRaftServiceServer(s, raft.NewRaftgRPCServer(kvServer.Raft()))
	kvraftpb.RegisterKVServiceServer(s, kvraft.NewKVgRPCServer(kvServer))

	fmt.Printf("Server %d starting at %s\n", *id, addrList[*id])
	if err := s.Serve(lis); err != nil {
		panic(err)
	}
}
