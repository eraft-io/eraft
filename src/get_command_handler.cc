/**
 * @file get_command_handler.cc
 * @author jay_jieliu@outlook.com
 * @brief
 * @version 0.1
 * @date 2023-06-25
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "command_handler.h"

EStatus GetCommandHandler::Execute(const std::vector<std::string>& params,
                                   Client*                         cli) {
  std::string leader_addr;
  leader_addr = cli->GetShardLeaderAddr(params[1]);
  ClientContext               op_context;
  eraftkv::ClientOperationReq op_req;
  auto                        kv_pair_ = op_req.add_kvs();
  kv_pair_->set_key(params[1]);
  kv_pair_->set_op_type(eraftkv::ClientOpType::Get);
  eraftkv::ClientOperationResp op_resp;
  std::string                  reply_buf;
  if (cli->kv_stubs_[leader_addr] != nullptr) {
    auto status_ = cli->kv_stubs_[leader_addr]->ProcessRWOperation(
        &op_context, op_req, &op_resp);
    if (status_.ok()) {
      reply_buf += "$";
      reply_buf += std::to_string(op_resp.ops(0).value().size());
      reply_buf += "\r\n";
      reply_buf += op_resp.ops(0).value();
      reply_buf += "\r\n";
    } else {
      reply_buf += "-ERR Server error\r\n";
    }
  } else {
    reply_buf += "-ERR Server error\r\n";
  }
  cli->reply_.PushData(reply_buf.c_str(), reply_buf.size());
  cli->SendPacket(cli->reply_);
  cli->_Reset();
  return EStatus::kOk;
}

GetCommandHandler::GetCommandHandler() {}

GetCommandHandler::~GetCommandHandler() {}