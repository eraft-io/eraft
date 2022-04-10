#include "proto_parser.h"
#include <assert.h>

// Example
// *3
// $3
// set
// $5
// qdqdw
// $6
// dqdwqd

ParseResult GetIntUntilCRLF(const char *&ptr, std::size_t nBytes, int &val)
{
	if (nBytes < 3)
		return ParseResult::wait;

	std::size_t i = 0;
	// bool negtive = false;
	// if (ptr[0] == '-')
	// {
	//     negtive = true;
	//     ++ i;
	// }
	// else if (ptr[0] == '+')
	// {
	//     ++ i;
	// }

	int value = 0;
	for (; i < nBytes; ++i) {
		if (isdigit(ptr[i])) {
			value *= 10;
			value += ptr[i] - '0';
		} else {
			if (ptr[i] != '\r' || (i + 1 < nBytes && ptr[i + 1] != '\n'))
				return ParseResult::error;

			if (i + 1 == nBytes)
				return ParseResult::wait;

			break;
		}
	}

	ptr += i;
	ptr += 2; // \r\n
	val = value;
	return ParseResult::ok;
}

void ProtoParser::Reset()
{
	multi_ = -1;
	paramLen_ = -1;
	numOfParam_ = 0;

	while (params_.size() > 3) {
		params_.pop_back();
	}
}

ParseResult ProtoParser::ParseRequest(const char *&ptr, const char *end)
{
	if (multi_ == -1) {
		auto parseRet = _ParseMulti(ptr, end, multi_);
		if (parseRet == ParseResult::error || multi_ < -1)
			return ParseResult::error;

		if (parseRet != ParseResult::ok)
			return ParseResult::wait;
	}

	return _ParseStrlist(ptr, end, params_);
}

ParseResult ProtoParser::_ParseMulti(const char *&ptr, const char *end, int &result)
{
	if (end - ptr < 3)
		return ParseResult::wait;

	if (*ptr != '*')
		return ParseResult::error;

	++ptr;

	return GetIntUntilCRLF(ptr, end - ptr, result);
}

ParseResult ProtoParser::_ParseStrlist(const char *&ptr, const char *end,
				       std::vector<std::string> &results)
{
	while (static_cast<int>(numOfParam_) < multi_) {
		if (results.size() < numOfParam_ + 1)
			results.resize(numOfParam_ + 1);

		auto parseRet = _ParseStr(ptr, end, results[numOfParam_]);

		if (parseRet == ParseResult::ok) {
			++numOfParam_;
		} else {
			return parseRet;
		}
	}

	results.resize(numOfParam_);
	return ParseResult::ok;
}

ParseResult ProtoParser::_ParseStr(const char *&ptr, const char *end, std::string &result)
{
	if (paramLen_ == -1) {
		auto parseRet = _ParseStrlen(ptr, end, paramLen_);
		if (parseRet == ParseResult::error || paramLen_ < -1)
			return ParseResult::error;

		if (parseRet != ParseResult::ok)
			return ParseResult::wait;
	}

	if (paramLen_ == -1) {
		result.clear(); // nil
		return ParseResult::ok;
	} else {
		return _ParseStrval(ptr, end, result);
	}
}

ParseResult ProtoParser::_ParseStrval(const char *&ptr, const char *end,
				      std::string &result)
{
	assert(paramLen_ >= 0);

	if (static_cast<int>(end - ptr) < paramLen_ + 2)
		return ParseResult::wait;

	auto tail = ptr + paramLen_;
	if (tail[0] != '\r' || tail[1] != '\n')
		return ParseResult::error;

	result.assign(ptr, tail - ptr);
	ptr = tail + 2;
	paramLen_ = -1;

	return ParseResult::ok;
}

ParseResult ProtoParser::_ParseStrlen(const char *&ptr, const char *end, int &result)
{
	if (end - ptr < 3)
		return ParseResult::wait;

	if (*ptr != '$')
		return ParseResult::error;

	++ptr;

	const auto ret = GetIntUntilCRLF(ptr, end - ptr, result);
	if (ret != ParseResult::ok)
		--ptr;

	return ret;
}
