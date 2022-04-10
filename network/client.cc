#include "client.h"
#include "executor.h"
#include "unbounded_buffer.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <sys/time.h>
#include <vector>

PacketLength Client::_HandlePacket(const char *start, std::size_t bytes)
{
	const char *const end = start + bytes;
	const char *ptr = start;
	auto parseRet = parser_.ParseRequest(start, end);
	if (parseRet != ParseResult::ok) {
		return static_cast<PacketLength>(ptr - start);
	}
	const auto &params = parser_.GetParams();
	if (params.empty())
		return static_cast<PacketLength>(ptr - start);

	std::string cmd(params[0]);
	std::transform(params[0].begin(), params[0].end(), cmd.begin(), ::tolower);

	timeval begin;
	gettimeofday(&begin, 0);
	long long beginUs_ = begin.tv_sec * 1000000 + begin.tv_usec;
	Error err = Error_ok;
	err = Executor::ExecuteCmd(params, &reply_);
	timeval endTime_;
	gettimeofday(&endTime_, 0);
	auto used = endTime_.tv_sec * 1000000 + endTime_.tv_usec - beginUs_;
	SendPacket(reply_);
	_Reset();
	return static_cast<PacketLength>(bytes);
}

Client::Client()
{
	_Reset();
}

void Client::_Reset()
{
	parser_.Reset();
	reply_.Clear();
}

void Client::OnConnect()
{
}
