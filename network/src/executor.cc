#include <network/executor.h>

#include <iostream>

Executor::Executor() {}

pmem::kv::db *Executor::engine_ = nullptr;

void Executor::Init(const std::vector<std::string> &params) {}

Error Executor::ExecuteCmd(const std::vector<std::string> &params,
                           UnboundedBuffer *reply) {
  if (params.empty()) {
    ReplyError(Error_param, reply);
    return Error_param;
  }

  auto it(CommandTable::s_handlers.find(params[0]));
  if (it == CommandTable::s_handlers.end()) {
    ReplyError(Error_unknowCmd, reply);
    return Error_unknowCmd;
  }

  const CommandInfo *info = it->second;
  if (!info->CheckParamsCount(static_cast<int>(params.size()))) {
    ReplyError(Error_param, reply);
    return Error_param;
  }

  return info->handler(params, reply);
}
