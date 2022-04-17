#pragma once

#include <network/command.h>
#include <network/common.h>
#include <network/unbounded_buffer.h>
#include  <libpmemkv.hpp>

const uint64_t PMEM_USED_SIZE_DEFAULT = 1024UL * 1024UL * 1024UL;

class Executor {

public:
	Executor();

	static void Init(const std::vector<std::string> &params);

	static Error ExecuteCmd(const std::vector<std::string> &params,
				UnboundedBuffer *reply = nullptr);

	static pmem::kv::db *engine_;
};
