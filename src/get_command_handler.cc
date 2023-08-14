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
#include "key_encode.h"

EStatus GetCommandHandler::Execute(const std::vector<std::string>& params,
                                   Client*                         cli) {
  std::string leader_addr;
  uint16_t    slot;
  leader_addr = cli->GetShardLeaderAddrAndSlot(params[1], &slot);
  TraceLog("DEBUG: send get request to leader ", leader_addr);
  ClientContext               op_context;
  eraftkv::ClientOperationReq op_req;
  auto                        kv_pair_ = op_req.add_kvs();
  kv_pair_->set_key(EncodeStringKey(slot, params[1]));
  kv_pair_->set_op_type(eraftkv::ClientOpType::Get);
  eraftkv::ClientOperationResp op_resp;
  std::string                  reply_buf;
  std::string*                 user_val = new std::string();
  if (cli->kv_stubs_[leader_addr] != nullptr) {
    auto status_ = cli->kv_stubs_[leader_addr]->ProcessRWOperation(
        &op_context, op_req, &op_resp);
    if (status_.ok()) {
      if (op_resp.ops(0).success()) {
        reply_buf += "$";
        char     flag;
        uint32_t expire;
        DecodeStringVal(
            op_resp.mutable_ops(0)->mutable_value(), &flag, &expire, user_val);
        reply_buf += std::to_string(user_val->size());
        reply_buf += "\r\n";
        reply_buf += *user_val;
        reply_buf += "\r\n";
      } else {
        reply_buf += "$-1\r\n";
      }
    } else {
      reply_buf += "-ERR Server error\r\n";
    }
  } else {
    reply_buf += "-ERR Server error\r\n";
  }
  cli->reply_.PushData(reply_buf.c_str(), reply_buf.size());
  cli->SendPacket(cli->reply_);
  cli->_Reset();
  delete user_val;
  return EStatus::kOk;
}

GetCommandHandler::GetCommandHandler() {}

GetCommandHandler::~GetCommandHandler() {}