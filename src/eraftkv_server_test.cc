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
 * @file eraftkv_server_test.cc
 * @author ERaftGroup
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

TEST(ERaftKvServerTest, ClientOperationReqRW) {
  auto chan_ =
      grpc::CreateChannel("127.0.0.1:8088", grpc::InsecureChannelCredentials());
  std::unique_ptr<ERaftKv::Stub> stub_(ERaftKv::NewStub(chan_));
  std::string test_key = RandomString::RandStr(64);
  std::string test_val = RandomString::RandStr(64);

  ClientContext               context;
  eraftkv::ClientOperationReq req;
  time_t                      time_in_sec;
  time(&time_in_sec);
  req.set_op_timestamp(static_cast<uint64_t>(time_in_sec));
  auto kv_pair = req.add_kvs();
  kv_pair->set_key(test_key);
  kv_pair->set_value(test_val);
  kv_pair->set_op_type(eraftkv::ClientOpType::Put);
  eraftkv::ClientOperationResp resp;
  auto status = stub_->ProcessRWOperation(&context, req, &resp);
  ASSERT_EQ(status.ok(), true);
  ASSERT_EQ(resp.ops_size(), 1);
  ASSERT_EQ(resp.ops(0).success(), true);

  ClientContext               context_get;
  eraftkv::ClientOperationReq req_get;
  time(&time_in_sec);
  req_get.set_op_timestamp(static_cast<uint64_t>(time_in_sec));
  auto kv_pair_ = req_get.add_kvs();
  kv_pair_->set_key(test_key);
  kv_pair_->set_op_type(eraftkv::ClientOpType::Get);
  eraftkv::ClientOperationResp resp_get;
  auto status_ = stub_->ProcessRWOperation(&context_get, req_get, &resp_get);
  ASSERT_EQ(status_.ok(), true);
  ASSERT_EQ(resp_get.ops_size(), 1);
  ASSERT_EQ(resp_get.ops(0).success(), true);
  ASSERT_EQ(resp_get.ops(0).value(), test_val);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}