//
// MIT License

// Copyright (c) 2022 eraft dev group

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

package main

import (
	"fmt"
	"net"
	"os"
	"os/signal"
	"strconv"
	"strings"
	"syscall"

	"github.com/eraft-io/eraft/metaserver"
	pb "github.com/eraft-io/eraft/raftpb"

	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
)

func main() {

	if len(os.Args) < 3 {
		fmt.Println("usage: server [nodeId] [configserveraddr1,configserveraddr2,configserveraddr3]")
		return
	}

	sigs := make(chan os.Signal, 1)
	signal.Notify(sigs, syscall.SIGINT, syscall.SIGTERM)

	node_id_str := os.Args[1]
	node_id, err := strconv.Atoi(node_id_str)
	if err != nil {
		panic(err)
	}
	meta_svr_addrs := strings.Split(os.Args[2], ",")
	cf_peer_map := make(map[int]string)
	for i, addr := range meta_svr_addrs {
		cf_peer_map[i] = addr
	}

	meta_svr := metaserver.MakeMetaServer(cf_peer_map, node_id)
	lis, err := net.Listen("tcp", cf_peer_map[node_id])
	if err != nil {
		fmt.Printf("failed to listen: %v", err)
		return
	}
	s := grpc.NewServer()

	pb.RegisterRaftServiceServer(s, meta_svr)

	sigChan := make(chan os.Signal, 1)

	signal.Notify(sigChan)

	go func() {
		sig := <-sigs
		fmt.Println(sig)
		meta_svr.Rf.CloseEndsConn()
		meta_svr.StopApply()
		os.Exit(-1)
	}()

	reflection.Register(s)
	err = s.Serve(lis)
	if err != nil {
		fmt.Printf("failed to serve: %v", err)
		return
	}
}
