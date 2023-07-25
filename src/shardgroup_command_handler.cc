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
 * @file shardgroup_command_handler.cc
 * @author jay_jieliu@outlook.com
 * @brief
 * @version 0.1
 * @date 2023-07-23
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "command_handler.h"
#include "eraftkv.grpc.pb.h"
#include "eraftkv.pb.h"
#include "util.h"

using eraftkv::ERaftKv;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

EStatus ShardGroupCommandHandler::Execute(
    const std::vector<std::string>& params,
    Client*                         cli) {
  std::vector<std::string> return_bufs;
  auto leader_chan = grpc::CreateChannel(cli->GetMetaLeaderAddr(),
                                         grpc::InsecureChannelCredentials());
  std::unique_ptr<ERaftKv::Stub> leader_stub(ERaftKv::NewStub(leader_chan));
  if (params[1] == "query") {
    ClientContext                   context;
    eraftkv::ClusterConfigChangeReq req;
    req.set_handle_server_type(eraftkv::HandleServerType::MetaServer);
    req.set_change_type(eraftkv::ChangeType::ShardsQuery);
    eraftkv::ClusterConfigChangeResp resp;
    auto st = leader_stub->ClusterConfigChange(&context, req, &resp);
    for (auto sg : resp.shard_group()) {
      return_bufs.push_back("shardgroup");
      return_bufs.push_back(std::to_string(sg.id()));
      return_bufs.push_back("servers");
      for (auto server : sg.servers()) {
        return_bufs.push_back(std::to_string(server.id()));
        return_bufs.push_back(server.address());
      }
    }
    // example: *2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n
    std::string reply_buf;
    reply_buf += "*";
    reply_buf += std::to_string(return_bufs.size());
    reply_buf += "\r\n";
    for (auto return_buf : return_bufs) {
      reply_buf += "$";
      reply_buf += std::to_string(return_buf.size());
      reply_buf += "\r\n";
      reply_buf += return_buf;
      reply_buf += "\r\n";
    }
    cli->reply_.PushData(reply_buf.c_str(), reply_buf.size());
    cli->SendPacket(cli->reply_);
    cli->_Reset();
  } else if (params[1] == "join") {
    // shardgroup join [shard id] [shard server addrs]
    ClientContext                   context;
    eraftkv::ClusterConfigChangeReq req;
    req.set_handle_server_type(eraftkv::HandleServerType::MetaServer);
    int shard_id = stoi(params[2]);
    req.set_shard_id(shard_id);
    req.mutable_shard_group()->set_id(shard_id);
    auto shard_server_addrs = StringUtil::Split(params[3], ',');
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
    auto        st = leader_stub->ClusterConfigChange(&context, req, &resp);
    std::string reply_buf;
    if (st.ok()) {
      reply_buf += "+OK\r\n";
    } else {
      reply_buf += "-ERR Server error\r\n";
    }
    cli->reply_.PushData(reply_buf.c_str(), reply_buf.size());
    cli->SendPacket(cli->reply_);
    cli->_Reset();
  } else if (params[1] == "move") {
    ClientContext                   context;
    eraftkv::ClusterConfigChangeReq req;
    int                             shard_id = stoi(params[2]);
    int                             slot_id = stoi(params[3]);
    req.set_change_type(eraftkv::ChangeType::SlotMove);
    req.set_shard_id(shard_id);
    auto to_move_sg = req.mutable_shard_group();
    to_move_sg->set_id(shard_id);
    auto new_slot = to_move_sg->add_slots();
    new_slot->set_id(slot_id);
    new_slot->set_slot_status(eraftkv::SlotStatus::Running);
    eraftkv::ClusterConfigChangeResp resp;
    auto        st = leader_stub->ClusterConfigChange(&context, req, &resp);
    std::string reply_buf;
    if (st.ok()) {
      reply_buf += "+OK\r\n";
    } else {
      reply_buf += "-ERR Server error\r\n";
    }
    cli->reply_.PushData(reply_buf.c_str(), reply_buf.size());
    cli->SendPacket(cli->reply_);
    cli->_Reset();
  } else if (params[1] == "leave") {
    ClientContext                   context;
    eraftkv::ClusterConfigChangeReq req;
    int                             shard_id = stoi(params[2]);
    req.set_change_type(eraftkv::ChangeType::ShardLeave);
    req.set_shard_id(shard_id);
    eraftkv::ClusterConfigChangeResp resp;
    auto        st = leader_stub->ClusterConfigChange(&context, req, &resp);
    std::string reply_buf;
    if (st.ok()) {
      reply_buf += "+OK\r\n";
    } else {
      reply_buf += "-ERR Server error\r\n";
    }
    cli->reply_.PushData(reply_buf.c_str(), reply_buf.size());
    cli->SendPacket(cli->reply_);
    cli->_Reset();
  }
  return EStatus::kOk;
}

ShardGroupCommandHandler::ShardGroupCommandHandler() {}

ShardGroupCommandHandler::~ShardGroupCommandHandler() {}