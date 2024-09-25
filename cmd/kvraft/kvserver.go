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
	"syscall"

	kvsvr "github.com/eraft-io/eraft/kvserver"
	pb "github.com/eraft-io/eraft/raftpb"
	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
)

func main() {
	if len(os.Args) < 2 {
		fmt.Println("usage: server [nodeId]")
		return
	}
	sigs := make(chan os.Signal, 1)

	signal.Notify(sigs, syscall.SIGINT, syscall.SIGTERM)

	nodeIdStr := os.Args[1]
	nodeId, err := strconv.Atoi(nodeIdStr)
	if err != nil {
		panic(err)
	}

	kvServer := kvsvr.MakeKvServer(nodeId)
	lis, err := net.Listen("tcp", kvsvr.PeersMap[nodeId])
	if err != nil {
		fmt.Printf("failed to listen: %v", err)
		return
	}
	fmt.Printf("server listen on: %s \n", kvsvr.PeersMap[nodeId])
	s := grpc.NewServer()
	pb.RegisterRaftServiceServer(s, kvServer)

	sigChan := make(chan os.Signal)

	signal.Notify(sigChan)

	go func() {
		sig := <-sigs
		fmt.Println(sig)
		kvServer.Rf.CloseEndsConn()
		os.Exit(-1)
	}()

	reflection.Register(s)
	err = s.Serve(lis)
	if err != nil {
		fmt.Printf("failed to serve: %v", err)
		return
	}
}
