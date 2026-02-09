package main

import (
	"flag"
	"fmt"
	"log"
	"net"
	"os"
	"strings"

	"github.com/eraft-io/eraft/raft"
	"github.com/eraft-io/eraft/raftpb"
	"github.com/eraft-io/eraft/shardctrler"
	"github.com/eraft-io/eraft/shardctrlerpb"
	"google.golang.org/grpc"
)

func main() {
	id := flag.Int("id", 0, "node id")
	cluster := flag.String("cluster", "127.0.0.1:50051,127.0.0.1:50052,127.0.0.1:50053", "cluster addresses")
	dbPath := flag.String("db", "data/shardctrler", "leveldb path")
	flag.Parse()

	addrs := strings.Split(*cluster, ",")
	if *id < 0 || *id >= len(addrs) {
		log.Fatalf("invalid id %d", *id)
	}

	peers := make([]raft.RaftPeer, len(addrs))
	for i, addr := range addrs {
		peers[i] = raft.NewRaftgRPCClient(addr)
	}

	// Ensure db directory exists
	nodeDbPath := fmt.Sprintf("%s_%d", *dbPath, *id)
	os.MkdirAll(nodeDbPath, 0755)

	persister := raft.MakeFilePersister(nodeDbPath)
	sc := shardctrler.StartServer(peers, *id, persister, nodeDbPath)

	lis, err := net.Listen("tcp", addrs[*id])
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	}

	s := grpc.NewServer()
	shardctrlerpb.RegisterShardCtrlerServiceServer(s, shardctrler.NewShardCtrlergRPCServer(sc))

	// Also register Raft gRPC server for inter-node communication
	raftpb.RegisterRaftServiceServer(s, raft.NewRaftgRPCServer(sc.Raft()))

	log.Printf("ShardCtrler server %d listening on %s", *id, addrs[*id])
	if err := s.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
}
