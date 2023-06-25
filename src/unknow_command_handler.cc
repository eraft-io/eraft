/**
 * @file unknow_command_handler.cc
 * @author jay_jieliu@outlook.com
 * @brief
 * @version 0.1
 * @date 2023-06-25
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "command_handler.h"

EStatus UnKnowCommandHandler::Execute(const std::vector<std::string>& params,
                                      Client*                         cli) {
  std::string reply_buf = "-ERR Unknown command\r\n";
  cli->reply_.PushData(reply_buf.c_str(), reply_buf.size());
  cli->SendPacket(cli->reply_);
  cli->_Reset();
  return EStatus::kOk;
}

UnKnowCommandHandler::UnKnowCommandHandler() {}

UnKnowCommandHandler::~UnKnowCommandHandler() {}