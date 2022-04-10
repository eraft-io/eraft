#include "command.h"
#include "common.h"
#include "executor.h"
#include "regex.h"
#include "unbounded_buffer.h"
#include <libpmemkv.hpp>
#include <sstream>

Error set(pmem::kv::db *engine, const std::vector<std::string> &params,
	  UnboundedBuffer *reply)
{
	auto s = engine->put(params[1], params[2]);
	if (s == pmem::kv::status::OK) {
		FormatOK(reply);
		return Error_ok;
	}
	ReplyError(Error_param, reply);
	return Error_param;
}

Error get(pmem::kv::db *engine, const std::vector<std::string> &params,
	  UnboundedBuffer *reply)
{
	std::string value;
	auto s = engine->get(params[1], &value);
	if (s == pmem::kv::status::NOT_FOUND) {
		FormatNull(reply);
		return Error_ok;
	}
	FormatBulk(value, reply);
	return Error_ok;
}

Error del(pmem::kv::db *engine, const std::vector<std::string> &params,
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

Error scan(pmem::kv::db *engine, const std::vector<std::string> &params,
	   UnboundedBuffer *reply)
{
	if (params.size() % 2 != 0) {
		ReplyError(Error_param, reply);
		return Error_param;
	}
	long cursor = 0;
	std::stringstream sstream(params[1]);
	sstream >> cursor;

	long count = -1;
	const char *pattern = nullptr;

	Error err = ParseScanOption(params, 2, count, pattern);
	if (err != Error_ok) {
		ReplyError(err, reply);
		return err;
	}
	// default count
	if (count < 0)
		count = 5;
	std::vector<std::string> matchKeys;
	std::string prefix = std::string(pattern);
	auto scanIter = engine->new_write_iterator();
	auto &s_it = scanIter.get_value();
	if (prefix.back() == '*') {
		prefix.pop_back();
	}
	auto seek_status = s_it.seek_higher_eq(prefix);
	long curNow = 0;
	uint64_t i = 0;
	while (cursor > 0 && s_it.next() == pmem::kv::status::OK) {
		cursor --;
		curNow ++;
	}
	do {
		i += 2;
		pmem::kv::result<pmem::kv::string_view> key_result = s_it.key();
		std::string current_key = key_result.get_value().data();
		if (qedis::glob_match(pattern, current_key.c_str())) {
			matchKeys.push_back(current_key);
			std::string current_value = s_it.read_range().get_value().data();
			matchKeys.push_back(current_value);
			curNow ++;
		}
	} while (s_it.next() == pmem::kv::status::OK && i < count);

	s_it.prev();
	if (s_it.next() != pmem::kv::status::OK) {
		curNow = 0;
	}

	PreFormatMultiBulk(2, reply);
	char buf[32];
	auto len = snprintf(buf, sizeof buf - 1, "%lu", curNow);
	FormatBulk(buf, len, reply); // cursor

	PreFormatMultiBulk(matchKeys.size(), reply);
	// keys
	for (const auto &s : matchKeys) {
		FormatBulk(s, reply);
	}
	return Error_ok;
}
