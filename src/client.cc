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

  if (parser_.GetParams()[0] == "info") {
    ClientContext                   context;
    eraftkv::ClusterConfigChangeReq req;
    req.set_change_type(eraftkv::ClusterConfigChangeType::Query);
    eraftkv::ClusterConfigChangeResp resp;

    auto status =
        stubs_.begin()->second->ClusterConfigChange(&context, req, &resp);
    std::string info_str;
    for (int i = 0; i < resp.shard_group(0).servers_size(); i++) {
      info_str += "server_id: ";
      info_str += std::to_string(resp.shard_group(0).servers(i).id());
      info_str += ",server_address: ";
      info_str += resp.shard_group(0).servers(i).address();
      resp.shard_group(0).servers(i).server_status() ==
              eraftkv::ServerStatus::Up
          ? info_str += ",status: Running"
          : info_str += ",status: Down";
      resp.shard_group(0).leader_id() == resp.shard_group(0).servers(i).id()
          ? info_str += ",Role: Leader"
          : info_str += ",Role: Follower";
      info_str += "\r\n";
    }
    std::string reply_buf;
    reply_buf += "$";
    reply_buf += std::to_string(info_str.size());
    reply_buf += "\r\n";
    reply_buf += info_str;
    reply_buf += "\r\n";
    reply_.PushData(reply_buf.c_str(), reply_buf.size());
    SendPacket(reply_);

    _Reset();
    return static_cast<PacketLength>(bytes);
  }

  reply_.PushData("+OK\r\n", 5);
  SendPacket(reply_);

  _Reset();
  return static_cast<PacketLength>(bytes);
}

Client::Client(std::string kv_addrs) {
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
