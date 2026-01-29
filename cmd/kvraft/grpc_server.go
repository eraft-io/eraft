package main

import (
	"flag"
	"net"
	"os"
	"os/signal"
	"strconv"
	"strings"
	"syscall"

	"github.com/eraft-io/eraft/kvraft"
	"github.com/eraft-io/eraft/labrpc"
	"github.com/eraft-io/eraft/logger"
	"github.com/eraft-io/eraft/raft"
	"github.com/eraft-io/eraft/raftpb"
	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
)

var nodeId = flag.Int("id", 0, "server node id")
var peerAddrs = flag.String("peers", "127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090", "server node peers")

func main() {
	flag.Parse()
	sigs := make(chan os.Signal, 1)
	signal.Notify(sigs, syscall.SIGINT, syscall.SIGTERM)
	PeerAddrs := strings.Split(*peerAddrs, ",")
	peerClients := []*labrpc.ClientEnd{}
	for _, peerAddr := range PeerAddrs {
		client := &labrpc.ClientEnd{}
		client.GrpcClient = labrpc.MakeGrpcClientEnd(uint64(*nodeId), peerAddr)
		peerClients = append(peerClients, client)
	}
	opts := &raft.PersisterOptions{
		OnFs:     true,
		RootPath: "./data/" + strconv.Itoa(*nodeId),
	}
	kvServer := kvraft.StartKVServer(peerClients, *nodeId, raft.MakePersister(opts), 102400)
	s := grpc.NewServer()
	raftpb.RegisterRaftServiceServer(s, kvServer)

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan)
	go func() {
		sig := <-sigs
		logger.ELogger().Sugar().Warn(sig)
		os.Exit(-1)
	}()

	reflection.Register(s)
	lis, err := net.Listen("tcp", PeerAddrs[*nodeId])
	if err != nil {
		logger.ELogger().Sugar().Errorf("failed to listen: %v", err)
		return
	}
	logger.ELogger().Sugar().Infof("starting server... on %s", PeerAddrs[*nodeId])
	err = s.Serve(lis)
	if err != nil {
		logger.ELogger().Sugar().Errorf("failed to serve: %v", err)
		return
	}
}
