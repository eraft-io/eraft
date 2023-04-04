/**
 * @file eraftkv_server_test.cc
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-04-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "eraftkv_server.h"

#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <iostream>

#include "eraftkv.grpc.pb.h"
#include "eraftkv.pb.h"

using eraftkv::ERaftKv;
using eraftkv::RequestVoteReq;
using eraftkv::RequestVoteResp;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

TEST(ERaftKvServerTest, voterpc) {
  pid_t fpid;
  fpid = fork();
  if (fpid < 0) {
    std::cout << "fork error!\n";
  }
  if (fpid == 0) {
    sleep(1);
    auto                           chan_ = grpc::CreateChannel("0.0.0.0:50051",
                                     grpc::InsecureChannelCredentials());
    std::unique_ptr<ERaftKv::Stub> stub_(ERaftKv::NewStub(chan_));
    ClientContext                  context;
    eraftkv::RequestVoteReq        req;
    eraftkv::RequestVoteResp       resp;
    auto status = stub_->RequestVote(&context, req, &resp);
    int  res = 0;
    if (status.ok())
      res = 1;
    ASSERT_EQ(res, 1);
  } else {
    // ERaftKvServerOptions options_;
    // options_.svr_addr = "0.0.0.0:50051";
    // ERaftKvServer server(options_);
    // server.BuildAndRunRpcServer();
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

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}