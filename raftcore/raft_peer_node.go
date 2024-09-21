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

package raftcore

import (
	"github.com/eraft-io/eraft/logger"
	raftpb "github.com/eraft-io/eraft/raftpb"
	"google.golang.org/grpc"
)

type RaftPeerNode struct {
	id             uint64
	addr           string
	conns          []*grpc.ClientConn
	raftServiceCli *raftpb.RaftServiceClient
}

func (rfEnd *RaftPeerNode) Id() uint64 {
	return rfEnd.id
}

func (rfEnd *RaftPeerNode) GetRaftServiceCli() *raftpb.RaftServiceClient {
	return rfEnd.raftServiceCli
}

func MakeRaftPeerNode(addr string, id uint64) *RaftPeerNode {
	conn, err := grpc.Dial(addr, grpc.WithInsecure())
	if err != nil {
		logger.ELogger().Sugar().DPanicf("faild to connect: %v", err)
	}
	conns := []*grpc.ClientConn{}
	conns = append(conns, conn)
	rpcClient := raftpb.NewRaftServiceClient(conn)
	return &RaftPeerNode{
		id:             id,
		addr:           addr,
		conns:          conns,
		raftServiceCli: &rpcClient,
	}
}

func (rfEnd *RaftPeerNode) CloseAllConn() {
	for _, conn := range rfEnd.conns {
		conn.Close()
	}
}
