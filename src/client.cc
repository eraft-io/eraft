/**
 * @file client.cc
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-06-17
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "client.h"

#include <stdint.h>
#include <sys/time.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "util.h"

PacketLength Client::_HandlePacket(const char *start, std::size_t bytes) {
  const char *const end = start + bytes;
  const char       *ptr = start;
  parser_.ParseRequest(ptr, end);

  CommandHandler *handler = nullptr;
  if (parser_.GetParams()[0] == "info") {
    handler = new InfoCommandHandler();
  } else if (parser_.GetParams()[0] == "set") {
    handler = new SetCommandHandler();
  } else if (parser_.GetParams()[0] == "get") {
    handler = new GetCommandHandler();
  } else if (parser_.GetParams()[0] == "shardgroup") {
    handler = new ShardGroupCommandHandler();
  } else {
    handler = new UnKnowCommandHandler();
  }

  handler->Execute(parser_.GetParams(), this);
  return static_cast<PacketLength>(bytes);
}

Client::Client(std::string meta_addrs)
    : leader_addr_(""), meta_addrs_(meta_addrs) {
  // init stub to meta server node
  auto meta_node_addrs = StringUtil::Split(meta_addrs, ',');
  for (auto meta_node_addr : meta_node_addrs) {
    TraceLog("DEBUG: init rpc link to ", meta_node_addr);
    auto chan_ =
        grpc::CreateChannel(meta_node_addr, grpc::InsecureChannelCredentials());
    std::unique_ptr<ERaftKv::Stub> stub_(ERaftKv::NewStub(chan_));
    this->meta_stubs_[meta_node_addr] = std::move(stub_);
  }
  // sync config
  SyncClusterConfig();
  _Reset();
}

void Client::_Reset() {
  parser_.Reset();
  reply_.Clear();
}

void Client::OnConnect() {}

std::string Client::GetMetaLeaderAddr() {
  std::string leader_address;
  auto        meta_node_addrs = StringUtil::Split(meta_addrs_, ',');
  for (auto meta_node_addr : meta_node_addrs) {
    ClientContext                         context;
    std::chrono::system_clock::time_point deadline =
        std::chrono::system_clock::now() + std::chrono::milliseconds(50);
    context.set_deadline(deadline);
    eraftkv::ClusterConfigChangeReq req;
    req.set_change_type(eraftkv::ChangeType::MembersQuery);
    eraftkv::ClusterConfigChangeResp resp;
    auto                             status =
        meta_stubs_[meta_node_addr]->ClusterConfigChange(&context, req, &resp);
    // server is down
    if (!status.ok()) {
      continue;
    }
    for (int i = 0; i < resp.shard_group(0).servers_size(); i++) {
      if (resp.shard_group(0).leader_id() ==
          resp.shard_group(0).servers(i).id()) {
        leader_address = resp.shard_group(0).servers(i).address();
      }
    }
  }
  return leader_address;
}

std::string Client::GetShardLeaderAddr(std::string partion_key) {
  std::string leader_address;
  int64_t     key_slot = -1;
  key_slot = HashUtil::CRC64(0, partion_key.c_str(), partion_key.size()) % 1024;
  TraceLog("DEBUG: partion key " + partion_key + " with slot ", key_slot);
  for (auto sg : cluster_conf_.shard_group()) {
    for (auto sl : sg.slots()) {
      if (key_slot == sl.id()) {
        // find sg leader addr
        for (auto server : sg.servers()) {
          ClientContext                         context;
          std::chrono::system_clock::time_point deadline =
              std::chrono::system_clock::now() + std::chrono::milliseconds(50);
          context.set_deadline(deadline);
          eraftkv::ClusterConfigChangeReq req;
          req.set_change_type(eraftkv::ChangeType::MembersQuery);
          eraftkv::ClusterConfigChangeResp resp;
          auto status = kv_stubs_[server.address()]->ClusterConfigChange(
              &context, req, &resp);
          if (!status.ok()) {
            continue;
          }
          for (int i = 0; i < resp.shard_group(0).servers_size(); i++) {
            if (resp.shard_group(0).leader_id() ==
                resp.shard_group(0).servers(i).id()) {
              leader_address = resp.shard_group(0).servers(i).address();
            }
          }
        }
      }
    }
  }
  return leader_address;
}

EStatus Client::SyncClusterConfig() {
  for (auto it = this->meta_stubs_.begin(); it != this->meta_stubs_.end();
       it++) {
    ClientContext                         context;
    std::chrono::system_clock::time_point deadline =
        std::chrono::system_clock::now() + std::chrono::milliseconds(50);
    context.set_deadline(deadline);
    eraftkv::ClusterConfigChangeReq req;
    req.set_change_type(eraftkv::ChangeType::ShardsQuery);
    auto status_ =
        it->second->ClusterConfigChange(&context, req, &cluster_conf_);
    if (!status_.ok()) {
      continue;
    }
    for (auto sg : cluster_conf_.shard_group()) {
      for (auto server : sg.servers()) {
        auto chan_ = grpc::CreateChannel(server.address(),
                                         grpc::InsecureChannelCredentials());
        std::unique_ptr<ERaftKv::Stub> stub_(ERaftKv::NewStub(chan_));
        this->kv_stubs_[server.address()] = std::move(stub_);
      }
    }
    if (status_.ok()) {
      return EStatus::kOk;
    }
  }
  return EStatus::kOk;
}
