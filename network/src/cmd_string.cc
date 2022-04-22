#include <network/command.h>
#include <network/common.h>
#include <network/executor.h>
#include <network/regex.h>
#include <network/unbounded_buffer.h>
#include <libpmemkv.hpp>
#include <sstream>

Error set(const std::vector<std::string> &params,
	  UnboundedBuffer *reply)
{
	ReplyError(Error_param, reply);
	return Error_param;
}

Error get(const std::vector<std::string> &params,
	  UnboundedBuffer *reply)
{

	return Error_ok;
}

Error del(const std::vector<std::string> &params,
	  UnboundedBuffer *reply)
{
	auto s = engine->remove(params[1]);
	if (s == pmem::kv::status::OK) {
		FormatInt(1, reply);
		return Error_ok;
	}
	FormatInt(0, reply);
	return Error_ok;
}

static Error ParseScanOption(const std::vector<std::string> &params, int start,
			     long &count, const char *&pattern)
{
	// SCAN 0 [cursor] match * [pattern] count 60
	count = -1;
	pattern = nullptr;

	for (std::size_t i = start; i < params.size(); i += 2) {
		if (params[i].size() == 5) {
			if (strncasecmp(params[i].c_str(), "match", 5) == 0) {
				if (!pattern) {
					pattern = params[i + 1].c_str();
					continue;
				}
			} else if (strncasecmp(params[i].c_str(), "count", 5) == 0) {
				if (count == -1) {
					std::stringstream sstream(params[i + 1]);
					sstream >> count;
					continue;
				}
			}
		}

		return Error_param;
	}

	return Error_ok;
}

Error scan(const std::vector<std::string> &params,
	   UnboundedBuffer *reply)
{

	return Error_ok;
}
