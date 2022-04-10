#pragma once

#include <strings.h>
#include <string>
#include <vector>

#define CRLF "\r\n"

enum Error
{
    Error_nop = -1,
    Error_ok = 0,
    Error_type = 1,
    Error_exist = 2,
    Error_notExists = 3,
    Error_param = 4,
    Error_unknowCmd = 5,
    Error_nan = 6,
    Error_syntax = 7,
};

extern struct ErrorInfo
{
    int len;
    const char* errorStr;
} g_errorInfo[];

class UnboundedBuffer;
void ReplyError(Error err, UnboundedBuffer* reply);

std::size_t FormatOK(UnboundedBuffer* reply);
std::size_t FormatBulk(const char* str, std::size_t len, UnboundedBuffer* reply);
std::size_t FormatBulk(const std::string& str, UnboundedBuffer* reply);
std::size_t PreFormatMultiBulk(std::size_t nBulk, UnboundedBuffer* reply);
std::size_t FormatMultiBulk(const std::vector<std::string> vs, UnboundedBuffer* reply);

std::size_t FormatNull(UnboundedBuffer* reply);

std::size_t FormatInt(long value, UnboundedBuffer* reply);


struct NocaseComp
{
    bool operator() (const std::string& s1, const std::string& s2) const
    {
        return strcasecmp(s1.c_str(), s2.c_str()) < 0;
    }

    bool operator() (const char* s1, const std::string& s2) const
    {
        return strcasecmp(s1, s2.c_str()) < 0;
    }

    bool operator() (const std::string& s1, const char* s2) const
    {
        return strcasecmp(s1.c_str(), s2) < 0;
    }
};
