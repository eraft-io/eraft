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
 * @file client.cc
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2024-03-31
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "client.h"

Client::Client() {
  this->metaserver_addrs_ = StringUtil::Split(DEFAULT_METASERVER_ADDRS, ',');
  bool is_chan_create_ok = false;
  for (auto metaserver_addr : this->metaserver_addrs_) {
    SPDLOG_INFO("init rpc link to {} ", metaserver_addr);
    auto chan_ = grpc::CreateChannel(metaserver_addr,
                                     grpc::InsecureChannelCredentials());
    if (chan_ != nullptr) {
      std::unique_ptr<ERaftKv::Stub> stub_(ERaftKv::NewStub(chan_));
      this->meta_svr_stubs_[metaserver_addr] = std::move(stub_);
      is_chan_create_ok = true;
    }
  }
  if (is_chan_create_ok) {
    this->UpdateMetaServerLeaderStub();
  }
  this->client_id_ = StringUtil::RandStr(16);
  this->command_id_ = 0;
}

Client::Client(std::string& metaserver_addrs) {
  this->metaserver_addrs_ = StringUtil::Split(metaserver_addrs, ',');
  for (auto metaserver_addr : this->metaserver_addrs_) {
    SPDLOG_INFO("init rpc link to {} ", metaserver_addr);
    auto                           chan_ = grpc::CreateChannel(metaserver_addr,
                                     grpc::InsecureChannelCredentials());
    std::unique_ptr<ERaftKv::Stub> stub_(ERaftKv::NewStub(chan_));
    this->meta_svr_stubs_[metaserver_addr] = std::move(stub_);
  }
  this->UpdateMetaServerLeaderStub();
  this->client_id_ = StringUtil::RandStr(16);
  this->command_id_ = 0;
}

bool Client::SetServerGroupSlotsToMeta(int64_t start_slot,
                                       int64_t end_slot,
                                       int64_t group_shard_id) {
  ClientContext                   context;
  eraftkv::ClusterConfigChangeReq req;
  req.set_change_type(eraftkv::ChangeType::SlotMove);
  req.set_shard_id(group_shard_id);
  req.set_client_id(this->client_id_);
  req.set_command_id(this->command_id_);
  auto to_move_sg = req.mutable_shard_group();
  to_move_sg->set_id(group_shard_id);
  for (int64_t i = start_slot; i <= end_slot; i++) {
    auto new_slot = to_move_sg->add_slots();
    new_slot->set_id(i);
    new_slot->set_slot_status(eraftkv::SlotStatus::Running);
  }
  eraftkv::ClusterConfigChangeResp resp;
  auto st = this->meta_leader_stub_->ClusterConfigChange(&context, req, &resp);
  assert(st.ok());
  this->command_id_++;
  return resp.success();
}

bool Client::AddServerGroupToMeta(int64_t     shard_id,
                                  int64_t     leader_id,
                                  std::string group_server_addrs) {
  ClientContext                   context;
  eraftkv::ClusterConfigChangeReq req;
  req.set_handle_server_type(eraftkv::HandleServerType::MetaServer);
  req.set_shard_id(shard_id);
  req.mutable_shard_group()->set_id(shard_id);
  req.set_client_id(this->client_id_);
  req.set_command_id(this->command_id_);
  auto shard_server_addrs = StringUtil::Split(group_server_addrs, ',');
  int  count = 0;
  for (auto shard_svr : shard_server_addrs) {
    auto svr = req.mutable_shard_group()->add_servers();
    svr->set_address(shard_svr);
    svr->set_id(count);
    count++;
    svr->set_server_status(eraftkv::ServerStatus::Up);
  }
  req.mutable_shard_group()->set_leader_id(leader_id);
  req.set_change_type(eraftkv::ChangeType::ShardJoin);
  eraftkv::ClusterConfigChangeResp resp;
  if (this->meta_leader_stub_ != nullptr) {
    auto st =
        this->meta_leader_stub_->ClusterConfigChange(&context, req, &resp);
    assert(st.ok());
  }
  this->command_id_++;
  return resp.success();
}

bool Client::RemoveServerGroupFromMeta(int64_t group_shard_id) {
  ClientContext                   context;
  eraftkv::ClusterConfigChangeReq req;
  req.set_client_id(this->client_id_);
  req.set_command_id(this->command_id_);
  req.set_change_type(eraftkv::ChangeType::ShardLeave);
  req.set_shard_id(group_shard_id);
  eraftkv::ClusterConfigChangeResp resp;
  auto st = this->meta_leader_stub_->ClusterConfigChange(&context, req, &resp);
  assert(st.ok());
  this->command_id_;
  return resp.success();
}

std::map<int64_t, eraftkv::ShardGroup> Client::GetServerGroupsFromMeta() {
  ClientContext                   context;
  eraftkv::ClusterConfigChangeReq req;
  req.set_client_id(this->client_id_);
  req.set_command_id(this->command_id_);
  req.set_handle_server_type(eraftkv::HandleServerType::MetaServer);
  req.set_change_type(eraftkv::ChangeType::ShardsQuery);
  eraftkv::ClusterConfigChangeResp resp;
  auto st = this->meta_leader_stub_->ClusterConfigChange(&context, req, &resp);
  assert(st.ok());
  this->command_id_;
  std::map<int64_t, eraftkv::ShardGroup> shardgroups;
  for (auto shard_group : resp.shard_group()) {
    shardgroups[shard_group.id()] = shard_group;
  }
  SPDLOG_INFO("get server groups resp {}", resp.DebugString());
  return std::move(shardgroups);
}

void Client::UpdateMetaServerLeaderStub() {
  for (auto meteserver_addr : this->metaserver_addrs_) {
    ClientContext                   context;
    eraftkv::ClusterConfigChangeReq req;
    req.set_change_type(eraftkv::ChangeType::MembersQuery);
    eraftkv::ClusterConfigChangeResp resp;
    auto status = this->meta_svr_stubs_[meteserver_addr]->ClusterConfigChange(
        &context, req, &resp);
    // server is down
    if (!status.ok()) {
      continue;
    }
    for (int i = 0; i < resp.shard_group(0).servers_size(); i++) {
      if (resp.shard_group(0).leader_id() ==
          resp.shard_group(0).servers(i).id()) {
        auto metaserver_leader_chan =
            grpc::CreateChannel(resp.shard_group(0).servers(i).address(),
                                grpc::InsecureChannelCredentials());
        this->meta_leader_stub_ =
            std::move(ERaftKv::NewStub(metaserver_leader_chan));
      }
    }
  }
}

void Client::UpdateKvServerLeaderStubByPartitionKey(std::string partition_key) {
  ClientContext                   context;
  eraftkv::ClusterConfigChangeReq req;
  req.set_handle_server_type(eraftkv::HandleServerType::MetaServer);
  req.set_change_type(eraftkv::ChangeType::ShardsQuery);
  eraftkv::ClusterConfigChangeResp cluster_config_resp;
  auto st = this->meta_leader_stub_->ClusterConfigChange(
      &context, req, &cluster_config_resp);
  if (!st.ok()) {
    SPDLOG_ERROR("call ClusterConfigChange error {}, {}",
                 st.error_code(),
                 st.error_message());
  }
  // cal key slot
  auto key_slot =
      HashUtil::CRC64(0, partition_key.c_str(), partition_key.size()) % 10;
  std::map<std::string, std::unique_ptr<ERaftKv::Stub> > kv_svr_stubs_;

  std::string kv_leader_address;
  for (auto sg : cluster_config_resp.shard_group()) {
    for (auto sl : sg.slots()) {
      if (key_slot == sl.id()) {
        // find sg leader addr
        for (auto server : sg.servers()) {
          auto new_chan_ = grpc::CreateChannel(
              server.address(), grpc::InsecureChannelCredentials());
          std::unique_ptr<ERaftKv::Stub> kv_stub(ERaftKv::NewStub(new_chan_));
          kv_svr_stubs_[server.address()] = std::move(kv_stub);
          SPDLOG_INFO("init rpc link to {} ", server.address());
          ClientContext                   query_kv_members_context;
          eraftkv::ClusterConfigChangeReq query_kv_members_req;
          query_kv_members_req.set_change_type(
              eraftkv::ChangeType::MembersQuery);
          eraftkv::ClusterConfigChangeResp query_kv_members_resp;
          auto status = kv_svr_stubs_[server.address()]->ClusterConfigChange(
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
  SPDLOG_INFO("kv server leader {}", kv_leader_address);
  auto kv_leader_chan = grpc::CreateChannel(kv_leader_address,
                                            grpc::InsecureChannelCredentials());
  this->kv_leader_stub_ = std::move(ERaftKv::NewStub(kv_leader_chan));
}

bool Client::PutKV(std::string k, std::string v) {
  ClientContext                op_context;
  eraftkv::ClientOperationReq  op_req;
  eraftkv::ClientOperationResp op_resp;
  op_req.set_client_id(this->client_id_);
  op_req.set_command_id(this->command_id_);
  auto kv_pair_ = op_req.add_kvs();
  kv_pair_->set_key(k);
  kv_pair_->set_value(v);
  kv_pair_->set_op_type(eraftkv::ClientOpType::Put);
  kv_pair_->set_op_sign(RandomNumber::Between(1, 10000));
  auto st =
      this->kv_leader_stub_->ProcessRWOperation(&op_context, op_req, &op_resp);
  if (st.ok()) {
    this->command_id_++;
    return true;
  } else {
    return false;
  }
}

std::pair<std::string, std::string> Client::GetKV(std::string k) {
  ClientContext                op_context;
  eraftkv::ClientOperationReq  op_req;
  eraftkv::ClientOperationResp op_resp;
  op_req.set_client_id(this->client_id_);
  op_req.set_command_id(this->command_id_);
  auto kv_pair_ = op_req.add_kvs();
  kv_pair_->set_key(k);
  kv_pair_->set_op_type(eraftkv::ClientOpType::Get);
  kv_pair_->set_op_sign(RandomNumber::Between(1, 10000));
  auto st =
      this->kv_leader_stub_->ProcessRWOperation(&op_context, op_req, &op_resp);
  assert(st.ok());
  this->command_id_++;
  auto key = op_resp.ops(0).key();
  auto val = op_resp.ops(0).value();
  return std::make_pair<std::string, std::string>(std::move(key),
                                                  std::move(val));
}

void Client::RunBench(int64_t N) {
  auto partition_key = StringUtil::RandStr(256);
  this->UpdateKvServerLeaderStubByPartitionKey(partition_key);
  for (int i = 0; i < N; i++) {
    auto value = StringUtil::RandStr(256);
    this->PutKV(partition_key, value);
  }
}

Client::~Client() {}
