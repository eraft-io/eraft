#include "command.h"

const CommandInfo CommandTable::s_info[] = {
	{"set", Attr_read, 3, &set},
	{"get", Attr_write, 2, &get},
	{"del", Attr_write, 2, &del},
	{"scan", Attr_read, -2, &scan},
};

std::map<std::string, const CommandInfo *, NocaseComp> CommandTable::s_handlers;

CommandTable::CommandTable()
{
	Init();
}

void CommandTable::Init()
{
	for (const auto &info : s_info) {
		s_handlers[info.cmd] = &info;
	}
}

const CommandInfo *CommandTable::GetCommandInfo(const std::string &cmd)
{
	auto it(s_handlers.find(cmd));
	if (it != s_handlers.end()) {
		return it->second;
	}
	return 0;
}

const CommandInfo *CommandTable::DelCommand(const std::string &cmd)
{
	auto it(s_handlers.find(cmd));
	if (it != s_handlers.end()) {
		auto p = it->second;
		s_handlers.erase(it);
		return p;
	}
	return nullptr;
}

bool CommandTable::AddCommand(const std::string &cmd, const CommandInfo *info)
{
	if (cmd.empty() || cmd == "\"\"")
		return true;

	return s_handlers.insert(std::make_pair(cmd, info)).second;
}

bool CommandInfo::CheckParamsCount(int nParams) const
{
	if (params > 0) {
		return params == nParams;
	} else {
		return nParams + params >= 0;
	}
}
