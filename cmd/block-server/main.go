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

package main

import (
	"flag"
	"fmt"
	"net"
	"os"
	"os/signal"
	"strings"
	"syscall"

	"net/http"
	_ "net/http/pprof"

	"github.com/eraft-io/eraft/pkg/blockserver"
	"github.com/eraft-io/eraft/pkg/consts"
	"github.com/eraft-io/eraft/pkg/log"
	pb "github.com/eraft-io/eraft/pkg/protocol"
	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
)

var nodeId = flag.Int("id", 0, "input this block server node id")
var nodeAddrs = flag.String("addrs", "127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090", "input block server node addrs")
var peerAddrs = flag.String("peers", "wellwood-blockserver-0.wellwood-blockserver:7088,wellwood-blockserver-1.wellwood-blockserver:7089,wellwood-blockserver-2.wellwood-blockserver:7090", "input block server node peers")
var monitorAddrs = flag.String("monitor_addrs", ":17088,:17089,:17090", "input block server monitor addrs")
var dataDir = flag.String("data_path", "./data", "input block server data path")
var groupId = flag.Int("gid", 0, "input this block server node id")
var metaNodeAddrs = flag.String("meta_addrs", "wellwood-metaserver-0.wellwood-metaserver:8088,wellwood-metaserver-1.wellwood-metaserver:8089,wellwood-metaserver-2.wellwood-metaserver:8090", "input block server node addrs")

func main() {
	flag.Parse()
	sigs := make(chan os.Signal, 1)
	signal.Notify(sigs, syscall.SIGINT, syscall.SIGTERM)
	blockSvrPeersMap := make(map[int]string)
	nodePeersArr := strings.Split(*peerAddrs, ",")
	for i, addr := range nodePeersArr {
		blockSvrPeersMap[i] = addr
	}
	blockSvrNodesMap := make(map[int]string)
	nodeAddrsArr := strings.Split(*nodeAddrs, ",")
	for i, addr := range nodeAddrsArr {
		blockSvrNodesMap[i] = addr
	}
	monitorSvrPeersMap := make(map[int]string)
	for i, addr := range strings.Split(*monitorAddrs, ",") {
		monitorSvrPeersMap[i] = addr
	}
	metaAddrsArr := strings.Split(*metaNodeAddrs, ",")
	blockServer := blockserver.MakeBlockServer(blockSvrPeersMap, *nodeId, *groupId, *dataDir, metaAddrsArr)
	svr := grpc.NewServer(grpc.MaxSendMsgSize(consts.MAX_GRPC_SEND_MSG_SIZE), grpc.MaxRecvMsgSize(consts.MAX_GRPC_RECV_MSG_SIZE))
	pb.RegisterRaftServiceServer(svr, blockServer)
	pb.RegisterFileBlockServiceServer(svr, blockServer)
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan)
	go func() {
		sig := <-sigs
		fmt.Printf("recived sig %d \n", sig)
		blockServer.StopAppling()
		os.Exit(-1)
	}()
	reflection.Register(svr)
	lis, err := net.Listen("tcp", blockSvrNodesMap[*nodeId])
	if err != nil {
		log.MainLogger.Error().Msgf("block server failed to listen: %v", err)
		return
	}
	go func() {
		if err := http.ListenAndServe(monitorSvrPeersMap[*nodeId], nil); err != nil {
			log.MainLogger.Error().Msgf("block server monitor failed to: %v", err)
		}
		os.Exit(0)
	}()
	log.MainLogger.Info().Msgf("block server success listen on: %s", blockSvrNodesMap[*nodeId])
	if err := svr.Serve(lis); err != nil {
		log.MainLogger.Error().Msgf("block server failed to serve: %v", err)
		return
	}
}
