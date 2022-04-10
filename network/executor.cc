#include "executor.h"
#include <iostream>

Executor::Executor()
{
}

pmem::kv::db *Executor::engine_ = nullptr;

void Executor::Init(const std::vector<std::string> &params)
{
	pmem::kv::config cfg;
	pmem::kv::status s = cfg.put_path(params[0]);
	if (s != pmem::kv::status::OK) {
		std::cout << "put_path error" << std::endl;
		return;
	}
	cfg.put_size(PMEM_USED_SIZE_DEFAULT);
	cfg.put_create_if_missing(true);
	engine_ = new pmem::kv::db();
	pmem::kv::db kv;
	s = engine_->open("stree", std::move(cfg));
	if (s != pmem::kv::status::OK) {
		std::cout << "open pmemkv error" << std::endl;
	}
}

Error Executor::ExecuteCmd(const std::vector<std::string> &params, UnboundedBuffer *reply)
{
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

	return info->handler(engine_, params, reply);
}
