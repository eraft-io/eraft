/**
 * @file info_command_handler.cc
 * @author jay_jieliu@outlook.com
 * @brief
 * @version 0.1
 * @date 2023-06-24
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "command_handler.h"
#include "util.h"

EStatus InfoCommandHandler::Execute(const std::vector<std::string>& params,
                                    Client*                         cli) {
  for (auto it = cli->meta_stubs_.begin(); it != cli->meta_stubs_.end(); it++) {
    ClientContext                         context;
    std::chrono::system_clock::time_point deadline =
        std::chrono::system_clock::now() + std::chrono::seconds(1);
    context.set_deadline(deadline);
    eraftkv::ClusterConfigChangeReq req;
    req.set_change_type(eraftkv::ChangeType::MetaMembersQuery);
    eraftkv::ClusterConfigChangeResp resp;
    auto status = it->second->ClusterConfigChange(&context, req, &resp);
    if (!status.ok()) {
      continue;
    }
    std::string info_str;
    for (int i = 0; i < resp.shard_group(0).servers_size(); i++) {
      info_str += "meta server: \r\n";
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

    cli->reply_.PushData(reply_buf.c_str(), reply_buf.size());
    cli->SendPacket(cli->reply_);

    cli->_Reset();
    if (status.ok()) {
      break;
    }
  }
  return EStatus::kOk;
}

InfoCommandHandler::InfoCommandHandler() {}

InfoCommandHandler::~InfoCommandHandler() {}
