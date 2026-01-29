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
	"github.com/eraft-io/eraft/shardkv"
	"github.com/eraft-io/eraft/shardkvpb"
	"google.golang.org/grpc"
)

func main() {
	id := flag.Int("id", 0, "node id within group")
	gid := flag.Int("gid", 0, "group id")
	cluster := flag.String("cluster", "localhost:6001,localhost:6002,localhost:6003", "comma separated addresses of nodes in THIS group")
	ctrlers := flag.String("ctrlers", "localhost:50051,localhost:50052,localhost:50053", "comma separated addresses of shardctrler nodes")
	dbPath := flag.String("db", "data/shardkv", "leveldb path prefix")
	flag.Parse()

	addrs := strings.Split(*cluster, ",")
	if *id < 0 || *id >= len(addrs) {
		log.Fatalf("invalid id %d", *id)
	}

	peers := make([]raft.RaftPeer, len(addrs))
	for i, addr := range addrs {
		peers[i] = raft.NewRaftgRPCClient(addr)
	}

	ctrlerList := strings.Split(*ctrlers, ",")

	// Ensure db directory exists
	nodeDbPath := fmt.Sprintf("%s_%d_%d", *dbPath, *gid, *id)
	os.MkdirAll(nodeDbPath, 0755)

	persister := raft.MakeFilePersister(nodeDbPath)
	kv := shardkv.StartServer(peers, *id, persister, -1, *gid, ctrlerList, nil, nodeDbPath)

	lis, err := net.Listen("tcp", addrs[*id])
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	}

	s := grpc.NewServer()
	shardkvpb.RegisterShardKVServiceServer(s, shardkv.NewShardKVgRPCServer(kv))
	raftpb.RegisterRaftServiceServer(s, raft.NewRaftgRPCServer(kv.Raft()))

	log.Printf("ShardKV group %d node %d listening on %s", *gid, *id, addrs[*id])
	if err := s.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
}
