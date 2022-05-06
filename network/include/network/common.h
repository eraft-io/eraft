// MIT License

// Copyright (c) 2022 eraft dev group

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ERAFT_COMMON_H_
#define ERAFT_COMMON_H_

#include <cstring>
#include <string>
#include <vector>

#define CRLF "\r\n"

enum Error {
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

extern struct ErrorInfo {
  int len;
  const char* errorStr;
} g_errorInfo[];

class UnboundedBuffer;
void ReplyError(Error err, UnboundedBuffer* reply);

std::size_t FormatOK(UnboundedBuffer* reply);
std::size_t FormatBulk(const char* str, std::size_t len,
                       UnboundedBuffer* reply);
std::size_t FormatBulk(const std::string& str, UnboundedBuffer* reply);
std::size_t PreFormatMultiBulk(std::size_t nBulk, UnboundedBuffer* reply);
std::size_t FormatMultiBulk(const std::vector<std::string> vs,
                            UnboundedBuffer* reply);

std::size_t FormatNull(UnboundedBuffer* reply);

std::size_t FormatInt(long value, UnboundedBuffer* reply);

struct NocaseComp {
  bool operator()(const std::string& s1, const std::string& s2) const {
    return strcasecmp(s1.c_str(), s2.c_str()) < 0;
  }

  bool operator()(const char* s1, const std::string& s2) const {
    return strcasecmp(s1, s2.c_str()) < 0;
  }

  bool operator()(const std::string& s1, const char* s2) const {
    return strcasecmp(s1.c_str(), s2) < 0;
  }
};

#endif
