// MIT License

// Copyright (c) 2023 ERaftGroup

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

/**
 * @file grpc_network_impl_test.cc
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-05-21
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "grpc_network_impl.h"

#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <iostream>

#include "eraftkv.grpc.pb.h"
#include "eraftkv.pb.h"
#include "eraftkv_server.h"
#include "raft_node.h"
#include "raft_server.h"
using eraftkv::ERaftKv;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

TEST(GrpcTest, TestInit) {
  pid_t fpid;
  fpid = fork();
  if (fpid < 0) {
    std::cout << "fork error!\n";
  }
  if (fpid == 0) {
    sleep(1);
    std::map<int64_t, std::string> peers_address;
    peers_address[1] = "0.0.0.0:50051";
    GRpcNetworkImpl grpcimpl;
    grpcimpl.InitPeerNodeConnections(peers_address);
    RaftNode    raftNode(1, NodeStateEnum::Running, 0, 0, "");
    RaftConfig  raftconf;
    RaftServer* raftServer =
        new RaftServer(raftconf, nullptr, nullptr, nullptr);
    eraftkv::RequestVoteReq req;
    auto status = grpcimpl.SendRequestVote(raftServer, &raftNode, &req);
    int  res = 0;
    if (EStatus::kOk == status)
      res = 1;
    ASSERT_EQ(res, 1);
  } else {
    ERaftKvServer service;
    grpc::EnableDefaultHealthCheckService(true);
    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051",
                             grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    sleep(5);
  }
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}