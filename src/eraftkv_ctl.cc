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
 * @file eraftkv_ctl.cc
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-06-10
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>
#include <time.h>

#include <iostream>

#include "eraftkv.grpc.pb.h"
#include "eraftkv.pb.h"
#include "util.h"

using eraftkv::ERaftKv;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define CTL_VERSION "v1.0.0"

enum op_code {
  QeuryGroups,
  AddGroup,
  SetSlot,
  RemoveGroup,
  PutKV,
  GetKV,
  RunBenchmark,
  Unknow
};

op_code hashit(std::string const& inString) {
  if (inString == "query_groups")
    return QeuryGroups;
  if (inString == "add_group")
    return AddGroup;
  if (inString == "set_slot")
    return SetSlot;
  if (inString == "remove_group")
    return RemoveGroup;
  if (inString == "put_kv")
    return PutKV;
  if (inString == "get_kv")
    return GetKV;
  if (inString == "run_bench")
    return RunBenchmark;
  return Unknow;
}

std::string GetServerGroupLeader(
    std::string                                            serverAddrs,
    std::map<std::string, std::unique_ptr<ERaftKv::Stub> > stubs) {
  auto node_addrs = StringUtil::Split(serverAddrs, ',');
  for (auto node_addr : node_addrs) {
    ClientContext context;

    eraftkv::ClusterConfigChangeReq req;
    req.set_change_type(eraftkv::ChangeType::MembersQuery);
    eraftkv::ClusterConfigChangeResp resp;
    auto status = stubs[node_addr]->ClusterConfigChange(&context, req, &resp);
    // server is down
    if (!status.ok()) {
      continue;
    }
    for (int i = 0; i < resp.shard_group(0).servers_size(); i++) {
      if (resp.shard_group(0).leader_id() ==
          resp.shard_group(0).servers(i).id()) {
        return resp.shard_group(0).servers(i).address();
      }
    }
  }
  return "";
}

std::string GetKVServerGroupLeader(
    uint64_t                                               key_slot,
    eraftkv::ClusterConfigChangeResp                       cluster_config_resp,
    std::map<std::string, std::unique_ptr<ERaftKv::Stub> > kv_svr_stubs) {
  std::string kv_leader_address;
  for (auto sg : cluster_config_resp.shard_group()) {
    for (auto sl : sg.slots()) {
      if (key_slot == sl.id()) {
        // find sg leader addr
        for (auto server : sg.servers()) {
          auto new_chan_ = grpc::CreateChannel(
              server.address(), grpc::InsecureChannelCredentials());
          std::unique_ptr<ERaftKv::Stub> kv_stub(ERaftKv::NewStub(new_chan_));
          kv_svr_stubs[server.address()] = std::move(kv_stub);
          SPDLOG_INFO("init rpc link to {} ", server.address());
          ClientContext                   query_kv_members_context;
          eraftkv::ClusterConfigChangeReq query_kv_members_req;
          query_kv_members_req.set_change_type(
              eraftkv::ChangeType::MembersQuery);
          eraftkv::ClusterConfigChangeResp query_kv_members_resp;
          auto status = kv_svr_stubs[server.address()]->ClusterConfigChange(
              &query_kv_members_context,
              query_kv_members_req,
              &query_kv_members_resp);
          if (!status.ok()) {
            continue;
          }
          for (int i = 0;
               i < query_kv_members_resp.shard_group(0).servers_size();
               i++) {
            if (query_kv_members_resp.shard_group(0).leader_id() ==
                query_kv_members_resp.shard_group(0).servers(i).id()) {
              kv_leader_address =
                  query_kv_members_resp.shard_group(0).servers(i).address();
            }
          }
        }
      }
    }
  }
  return kv_leader_address;
}

std::unique_ptr<ERaftKv::Stub> GetKvLeaderStubByPartitionKey(
    std::string                    partition_key,
    std::unique_ptr<ERaftKv::Stub> meta_leader_stub) {
  ClientContext                   context;
  eraftkv::ClusterConfigChangeReq req;
  req.set_handle_server_type(eraftkv::HandleServerType::MetaServer);
  req.set_change_type(eraftkv::ChangeType::ShardsQuery);
  eraftkv::ClusterConfigChangeResp cluster_config_resp;
  auto                             st = meta_leader_stub->ClusterConfigChange(
      &context, req, &cluster_config_resp);
  if (!st.ok()) {
    SPDLOG_ERROR("call ClusterConfigChange error {}, {}",
                 st.error_code(),
                 st.error_message());
    return nullptr;
  }
  std::string kv_leader_address;
  // cal key slot
  auto key_slot =
      HashUtil::CRC64(0, partition_key.c_str(), partition_key.size()) % 10;
  std::map<std::string, std::unique_ptr<ERaftKv::Stub> > kv_svr_stubs_;
  kv_leader_address = GetKVServerGroupLeader(
      key_slot, cluster_config_resp, std::move(kv_svr_stubs_));
  SPDLOG_INFO("kv server leader {}", kv_leader_address);
  auto kv_leader_chan = grpc::CreateChannel(kv_leader_address,
                                            grpc::InsecureChannelCredentials());
  return ERaftKv::NewStub(kv_leader_chan);
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << "Welcome to eraftkv-ctl, Copyright (c) 2023 ERaftGroup "
              << CTL_VERSION << std::endl;
    std::cout
        << "put_kv: ./eraftkv-ctl [metaserver addresses] put_kv [key] [value]"
        << std::endl;
    std::cout << "get_kv: ./eraftkv-ctl [metaserver addresses] get_kv [key]"
              << std::endl;
    std::cout << "add_group: ./eraftkv-ctl [metaserver addresses] add_group "
                 "[groupid] [group server addresses]"
              << std::endl;
    std::cout << "remove_group: ./eraftkv-ctl [metaserver addresses] "
                 "remove_group [group id] [node id]"
              << std::endl;
    std::cout
        << "query_groups: ./eraftkv-ctl [metaserver addresses] query_groups"
        << std::endl;
    std::cout << "set_slot: ./eraftkv-ctl [metaserver addresses] set_slot "
                 "[groupid] [startSlot-endSlot]"
              << std::endl;
    exit(-1);
  }

  std::string metaserver_addrs = std::string(argv[1]);
  // link to metaservers
  std::map<std::string, std::unique_ptr<ERaftKv::Stub> > meta_svr_stubs_;
  auto meta_node_addrs = StringUtil::Split(metaserver_addrs, ',');
  for (auto meta_node_addr : meta_node_addrs) {
    SPDLOG_INFO("init rpc link to {} ", meta_node_addr);
    auto chan_ =
        grpc::CreateChannel(meta_node_addr, grpc::InsecureChannelCredentials());
    std::unique_ptr<ERaftKv::Stub> stub_(ERaftKv::NewStub(chan_));
    meta_svr_stubs_[meta_node_addr] = std::move(stub_);
  }
  std::string meta_leader_addr =
      GetServerGroupLeader(metaserver_addrs, std::move(meta_svr_stubs_));
  SPDLOG_INFO("meta server leader {}", meta_leader_addr);
  auto leader_chan =
      grpc::CreateChannel(meta_leader_addr, grpc::InsecureChannelCredentials());
  std::unique_ptr<ERaftKv::Stub> meta_leader_stub(
      ERaftKv::NewStub(leader_chan));
  std::string cmd = std::string(argv[2]);
  switch (hashit(cmd)) {
    case AddGroup: {
      ClientContext                   context;
      eraftkv::ClusterConfigChangeReq req;
      req.set_handle_server_type(eraftkv::HandleServerType::MetaServer);
      int shard_id = stoi(std::string(argv[3]));
      req.set_shard_id(shard_id);
      req.mutable_shard_group()->set_id(shard_id);
      auto shard_server_addrs = StringUtil::Split(std::string(argv[4]), ',');
      int  count = 0;
      for (auto shard_svr : shard_server_addrs) {
        auto svr = req.mutable_shard_group()->add_servers();
        svr->set_address(shard_svr);
        svr->set_id(count);
        count++;
        svr->set_server_status(eraftkv::ServerStatus::Up);
      }
      req.set_change_type(eraftkv::ChangeType::ShardJoin);
      eraftkv::ClusterConfigChangeResp resp;
      auto st = meta_leader_stub->ClusterConfigChange(&context, req, &resp);
      assert(st.ok());
      std::cout << resp.DebugString() << std::endl;
      break;
    }
    case QeuryGroups: {
      ClientContext                   context;
      eraftkv::ClusterConfigChangeReq req;
      req.set_handle_server_type(eraftkv::HandleServerType::MetaServer);
      req.set_change_type(eraftkv::ChangeType::ShardsQuery);
      eraftkv::ClusterConfigChangeResp resp;
      auto st = meta_leader_stub->ClusterConfigChange(&context, req, &resp);
      if (!st.ok()) {
        SPDLOG_ERROR("call ClusterConfigChange error {}, {}",
                     st.error_code(),
                     st.error_message());
      }
      std::cout << resp.DebugString() << std::endl;
      break;
    }
    case SetSlot: {
      auto slot_range_args = StringUtil::Split(std::string(argv[4]), '-');
      ClientContext                   context;
      std::string                     reply_buf;
      eraftkv::ClusterConfigChangeReq req;
      int                             shard_id = stoi(std::string(argv[3]));
      req.set_change_type(eraftkv::ChangeType::SlotMove);
      req.set_shard_id(shard_id);
      auto to_move_sg = req.mutable_shard_group();
      to_move_sg->set_id(shard_id);
      // support move range slot shardgroup move 1 [startslot-endslot]
      if (slot_range_args.size() == 2) {
        try {
          int64_t start_slot =
              static_cast<int64_t>(std::stoi(slot_range_args[0]));
          int64_t end_slot =
              static_cast<int64_t>(std::stoi(slot_range_args[1]));
          for (int64_t i = start_slot; i <= end_slot; i++) {
            auto new_slot = to_move_sg->add_slots();
            new_slot->set_id(i);
            new_slot->set_slot_status(eraftkv::SlotStatus::Running);
          }
        } catch (const std::invalid_argument& e) {
          SPDLOG_ERROR("invalid_argument {}", e.what());
        }
      }
      eraftkv::ClusterConfigChangeResp resp;
      auto st = meta_leader_stub->ClusterConfigChange(&context, req, &resp);
      if (!st.ok()) {
        SPDLOG_ERROR("call ClusterConfigChange error {}, {}",
                     st.error_code(),
                     st.error_message());
      }
      std::cout << resp.DebugString() << std::endl;
      break;
    }
    case RemoveGroup: {
      ClientContext                   context;
      eraftkv::ClusterConfigChangeReq req;
      int                             shard_id = stoi(std::string(argv[3]));
      req.set_change_type(eraftkv::ChangeType::ShardLeave);
      req.set_shard_id(shard_id);
      eraftkv::ClusterConfigChangeResp resp;
      auto st = meta_leader_stub->ClusterConfigChange(&context, req, &resp);
      if (!st.ok()) {
        SPDLOG_ERROR("call ClusterConfigChange error {}, {}",
                     st.error_code(),
                     st.error_message());
      }
      std::cout << resp.DebugString() << std::endl;
      break;
    }
    case RunBenchmark: {
      int64_t N =
              static_cast<int64_t>(std::stoi(std::string(argv[3])));
      for (int i = 0; i < N; i ++) {
        auto partition_key = StringUtil::RandStr(256);
        auto kv_leader_stub = GetKvLeaderStubByPartitionKey(
            partition_key, std::make_unique<ERaftKv::Stub>(*meta_leader_stub));
        auto                         value = StringUtil::RandStr(256);
        ClientContext                op_context;
        eraftkv::ClientOperationReq  op_req;
        eraftkv::ClientOperationResp op_resp;
        auto                         kv_pair_ = op_req.add_kvs();
        kv_pair_->set_key(partition_key);
        kv_pair_->set_value(value);
        kv_pair_->set_op_type(eraftkv::ClientOpType::Put);
        kv_pair_->set_op_sign(RandomNumber::Between(1, 10000));
        kv_leader_stub->ProcessRWOperation(&op_context, op_req, &op_resp);
        std::cout << op_resp.DebugString() << std::endl;
      }
      break;
    }
    case PutKV: {
      auto partition_key = std::string(std::string(argv[3]));
      auto value = std::string(std::string(argv[4]));
      auto kv_leader_stub = GetKvLeaderStubByPartitionKey(
          partition_key, std::move(meta_leader_stub));
      ClientContext                op_context;
      eraftkv::ClientOperationReq  op_req;
      eraftkv::ClientOperationResp op_resp;
      auto                         kv_pair_ = op_req.add_kvs();
      kv_pair_->set_key(partition_key);
      kv_pair_->set_value(value);
      kv_pair_->set_op_type(eraftkv::ClientOpType::Put);
      kv_pair_->set_op_sign(RandomNumber::Between(1, 10000));
      kv_leader_stub->ProcessRWOperation(&op_context, op_req, &op_resp);
      std::cout << op_resp.DebugString() << std::endl;
      break;
    }
    case GetKV: {
      auto partition_key = std::string(std::string(argv[3]));
      auto kv_leader_stub = GetKvLeaderStubByPartitionKey(
          partition_key, std::move(meta_leader_stub));
      ClientContext                op_context;
      eraftkv::ClientOperationReq  op_req;
      eraftkv::ClientOperationResp op_resp;
      auto                         kv_pair_ = op_req.add_kvs();
      kv_pair_->set_key(partition_key);
      kv_pair_->set_op_type(eraftkv::ClientOpType::Get);
      kv_pair_->set_op_sign(RandomNumber::Between(1, 10000));
      kv_leader_stub->ProcessRWOperation(&op_context, op_req, &op_resp);
      std::cout << op_resp.DebugString() << std::endl;
      break;
    }
    default:
      break;
  }
  return 0;
}
