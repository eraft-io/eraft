/**
 * @file del_command_handler.cc
 * @author jay_jieliu@outlook.com
 * @brief 
 * @version 0.1
 * @date 2023-07-28
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "command_handler.h"
#include "key_encode.h"

EStatus DelCommandHandler::Execute(const std::vector<std::string>& params,
                                   Client*                         cli) {
  std::string leader_addr;
  uint16_t    slot;
  leader_addr = cli->GetShardLeaderAddrAndSlot(params[1], &slot);
  TraceLog("DEBUG: send del request to leader ", leader_addr);
  ClientContext                op_context;
  eraftkv::ClientOperationReq  op_req;
  eraftkv::ClientOperationResp op_resp;
  auto                         kv_pair_ = op_req.add_kvs();
  std::string                  encode_key = EncodeStringKey(slot, params[1]);
  kv_pair_->set_key(encode_key);
  kv_pair_->set_op_type(eraftkv::ClientOpType::Del);
  kv_pair_->set_op_count(RandomNumber::Between(1,10000));
  std::string reply_buf;
  if (cli->kv_stubs_[leader_addr] != nullptr) {
    auto status_ = cli->kv_stubs_[leader_addr]->ProcessRWOperation(
        &op_context, op_req, &op_resp);
    if (status_.ok()) {
      reply_buf += "+OK\r\n";
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

DelCommandHandler::DelCommandHandler() {}

DelCommandHandler::~DelCommandHandler() {}
