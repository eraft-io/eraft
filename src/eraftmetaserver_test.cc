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
 * @file eraftmetaserver_tests.cc
 * @author jay_jieliu@outlook.com
 * @brief
 * @version 0.1
 * @date 2023-07-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>
#include <time.h>
#include <unistd.h>

#include <iostream>
#include <thread>

#include "eraftkv.grpc.pb.h"
#include "eraftkv.pb.h"
#include "util.h"

using eraftkv::ERaftKv;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define SERVER_ADDR "172.18.0.2:8088"

TEST(EraftMetaServerTests, TestMetaBasicOp) {
  auto chan_ =
      grpc::CreateChannel(SERVER_ADDR, grpc::InsecureChannelCredentials());
  std::unique_ptr<ERaftKv::Stub>  stub_(ERaftKv::NewStub(chan_));
  ClientContext                   qm_conext;
  eraftkv::ClusterConfigChangeReq qm_req;
  qm_req.set_change_type(eraftkv::ChangeType::MetaMembersQuery);
  eraftkv::ClusterConfigChangeResp qm_resp;
  auto        status = stub_->ClusterConfigChange(&qm_conext, qm_req, &qm_resp);
  std::string leader_address = "";
  for (auto s : qm_resp.shard_group(0).servers()) {
    if (s.id() == qm_resp.shard_group(0).leader_id()) {
      leader_address = s.address();
    }
  }

  ASSERT_FALSE(leader_address.empty());

  auto leader_chan =
      grpc::CreateChannel(leader_address, grpc::InsecureChannelCredentials());
  std::unique_ptr<ERaftKv::Stub> leader_stub(ERaftKv::NewStub(leader_chan));

  ClientContext                   add_sg_context;
  eraftkv::ClusterConfigChangeReq req;
  req.set_handle_server_type(eraftkv::HandleServerType::MetaServer);
  req.set_shard_id(1);
  req.mutable_shard_group()->set_id(1);
  req.mutable_shard_group()->set_leader_id(0);
  auto svr0 = req.mutable_shard_group()->add_servers();
  svr0->set_address("172.18.0.10:8088");
  svr0->set_id(0);
  svr0->set_server_status(eraftkv::ServerStatus::Up);
  auto svr1 = req.mutable_shard_group()->add_servers();
  svr1->set_address("172.18.0.11:8089");
  svr1->set_id(1);
  svr1->set_server_status(eraftkv::ServerStatus::Up);
  auto svr2 = req.mutable_shard_group()->add_servers();
  svr2->set_address("172.18.0.12:8090");
  svr2->set_id(2);
  svr2->set_server_status(eraftkv::ServerStatus::Up);
  req.set_change_type(eraftkv::ChangeType::ShardJoin);
  eraftkv::ClusterConfigChangeResp resp;
  auto status1 = leader_stub->ClusterConfigChange(&add_sg_context, req, &resp);
  ASSERT_EQ(status1.ok(), true);
  std::this_thread::sleep_for(std::chrono::seconds(2));

  ClientContext                   query_context;
  eraftkv::ClusterConfigChangeReq query_req;
  query_req.set_handle_server_type(eraftkv::HandleServerType::MetaServer);
  query_req.set_change_type(eraftkv::ChangeType::ShardsQuery);
  eraftkv::ClusterConfigChangeResp query_resp;
  auto                             status2 =
      leader_stub->ClusterConfigChange(&query_context, query_req, &query_resp);
  ASSERT_EQ(status2.ok(), true);
  ASSERT_EQ(query_resp.shard_group_size(), 1);
  ASSERT_EQ(query_resp.shard_group(0).servers_size(), 3);
  ASSERT_EQ(query_resp.shard_group(0).id(), 1);
  TraceLog("DEBUG: cluster config resp -> ", query_resp.DebugString());

  ClientContext                   move_context;
  eraftkv::ClusterConfigChangeReq move_req;
  move_req.set_change_type(eraftkv::ChangeType::SlotMove);
  move_req.set_shard_id(1);
  auto to_move_sg = move_req.mutable_shard_group();
  to_move_sg->set_id(1);

  for (int64_t i = 0; i < 1024; i++) {
    auto new_slot = to_move_sg->add_slots();
    new_slot->set_id(i);
    new_slot->set_slot_status(eraftkv::SlotStatus::Running);
  }

  eraftkv::ClusterConfigChangeResp move_resp;
  auto                             status3 =
      leader_stub->ClusterConfigChange(&move_context, move_req, &move_resp);
  ASSERT_EQ(status3.ok(), true);

  std::this_thread::sleep_for(std::chrono::seconds(2));
  ClientContext query_context_;
  auto          status4 =
      leader_stub->ClusterConfigChange(&query_context_, query_req, &query_resp);
  //   TraceLog("DEBUG: cluster config after move resp -> ",
  //            query_resp.DebugString());
  ASSERT_EQ(status4.ok(), true);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
