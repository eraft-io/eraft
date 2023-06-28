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
  } else {
    handler = new UnKnowCommandHandler();
  }

  handler->Execute(parser_.GetParams(), this);
  return static_cast<PacketLength>(bytes);
}

Client::Client(std::string kv_addrs) : leader_addr_("") {
  // init stub to kv server node
  auto kv_node_addrs = StringUtil::Split(kv_addrs, ',');
  for (auto kv_node_addr : kv_node_addrs) {
    auto chan_ =
        grpc::CreateChannel(kv_node_addr, grpc::InsecureChannelCredentials());
    std::unique_ptr<ERaftKv::Stub> stub_(ERaftKv::NewStub(chan_));
    this->stubs_[kv_node_addr] = std::move(stub_);
  }
  _Reset();
}

void Client::_Reset() {
  parser_.Reset();
  reply_.Clear();
}

void Client::OnConnect() {}

std::string Client::GetLeaderAddr() {
  ClientContext                   context;
  eraftkv::ClusterConfigChangeReq req;
  req.set_change_type(eraftkv::ChangeType::Query);
  eraftkv::ClusterConfigChangeResp resp;
  auto                             status =
      this->stubs_.begin()->second->ClusterConfigChange(&context, req, &resp);
  std::string leader_addr = "";
  for (int i = 0; i < resp.shard_group(0).servers_size(); i++) {
    if (resp.shard_group(0).leader_id() ==
        resp.shard_group(0).servers(i).id()) {
      leader_addr = resp.shard_group(0).servers(i).address();
    }
  }
  return leader_addr;
}
