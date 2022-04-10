#pragma once

#include "common.h"
#include <libpmemkv.hpp>
#include <map>
#include <string>
#include <vector>

enum CommandAttr {
	Attr_read = 0x1,
	Attr_write = 0x1 << 1,
};

class UnboundedBuffer;
using CommandHandler = Error(pmem::kv::db *engine, const std::vector<std::string> &params,
			     UnboundedBuffer *reply);

CommandHandler set;
CommandHandler get;
CommandHandler del;
CommandHandler scan;
CommandHandler info;

struct CommandInfo {
	std::string cmd;
	int attr;
	int params;
	CommandHandler *handler;
	bool CheckParamsCount(int nParams) const;
};

class Executor;

class CommandTable {

public:
	friend class Executor;

public:
	CommandTable();

	static void Init();

	static const CommandInfo *GetCommandInfo(const std::string &cmd);

	static bool AddCommand(const std::string &cmd, const CommandInfo *info);

	static const CommandInfo *DelCommand(const std::string &cmd);

	CommandHandler cmdlist;

private:
	static std::map<std::string, const CommandInfo *, NocaseComp> s_handlers;

	static const CommandInfo s_info[];
};
