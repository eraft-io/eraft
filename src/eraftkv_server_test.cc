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
#include <time.h>
#include <unistd.h>

#include <iostream>

#include "eraftkv.grpc.pb.h"
#include "eraftkv.pb.h"
#include "util.h"

using eraftkv::ERaftKv;
using eraftkv::RequestVoteReq;
using eraftkv::RequestVoteResp;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

TEST(ERaftKvServerTest, ClientOperationReq) {
  auto chan_ =
      grpc::CreateChannel("127.0.0.1:8088", grpc::InsecureChannelCredentials());
  std::unique_ptr<ERaftKv::Stub> stub_(ERaftKv::NewStub(chan_));
  for (int i = 0; i < 100; i++) {
    ClientContext               context;
    eraftkv::ClientOperationReq req;
    time_t                      time_in_sec;
    time(&time_in_sec);
    req.set_op_timestamp(static_cast<uint64_t>(time_in_sec));
    auto kv_pair = req.add_kvs();
    kv_pair->set_key(RandomString::RandStr(64));
    kv_pair->set_value(RandomString::RandStr(64));
    kv_pair->set_op_type(eraftkv::ClientOpType::Put);
    eraftkv::ClientOperationResp resp;
    auto status = stub_->ProcessRWOperation(&context, req, &resp);
  }
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}