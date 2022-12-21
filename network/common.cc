// Copyright 2022 The uhp-sql Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "common.h"

#include <vector>

#include "unbounded_buffer.h"

struct ErrorInfo g_errorInfo[] = {
    {sizeof "+OK\r\n" - 1, "+OK\r\n"},
    {sizeof "-ERR Operation against a key holding the wrong kind of value\r\n" -
         1,
     "-ERR Operation against a key holding the wrong kind of value\r\n"},
    {sizeof "-ERR already exist" - 1, "-ERR already exist"},
    {sizeof "-ERR no such key\r\n" - 1, "-ERR no such key\r\n"},
    {sizeof "-ERR wrong number of arguments\r\n" - 1,
     "-ERR wrong number of arguments\r\n"},
    {sizeof "-ERR Unknown command\r\n" - 1, "-ERR Unknown command\r\n"},
    {sizeof "-ERR value is not an integer or out of range\r\n" - 1,
     "-ERR value is not an integer or out of range\r\n"},
    {sizeof "-ERR syntax error\r\n" - 1, "-ERR syntax error\r\n"},
};

void ReplyError(Error err, UnboundedBuffer *reply) {
  if (!reply) {
    return;
  }
  const ErrorInfo &info = g_errorInfo[err];

  reply->PushData(info.errorStr, info.len);
}

size_t FormatOK(UnboundedBuffer *reply) {
  if (!reply) return 0;

  size_t oldSize = reply->ReadableSize();
  reply->PushData("+OK" CRLF, 5);

  return reply->ReadableSize() - oldSize;
}

size_t FormatBulk(const char *str, std::size_t len, UnboundedBuffer *reply) {
  if (!reply) return 0;

  size_t oldSize = reply->ReadableSize();
  reply->PushData("$", 1);
  // encode len
  char val[32];
  int tmp = snprintf(val, sizeof val - 1, "%lu" CRLF, len);
  reply->PushData(val, tmp);
  // encode data
  if (str && len > 0) {
    reply->PushData(str, len);
  }
  reply->PushData(CRLF, 2);
  return reply->ReadableSize() - oldSize;
}

size_t FormatBulk(const std::string &str, UnboundedBuffer *reply) {
  return FormatBulk(str.c_str(), str.size(), reply);
}

size_t PreFormatMultiBulk(std::size_t nBulk, UnboundedBuffer *reply) {
  if (!reply) return 0;

  size_t oldSize = reply->ReadableSize();
  reply->PushData("*", 1);

  // encode multi num
  char val[32];
  int tmp = snprintf(val, sizeof val - 1, "%lu" CRLF, nBulk);
  reply->PushData(val, tmp);

  return reply->ReadableSize() - oldSize;
}

size_t FormatMultiBulk(const std::vector<std::string> vs,
                       UnboundedBuffer *reply) {
  size_t size = 0;
  for (const auto &s : vs) size += FormatBulk(s, reply);

  return size;
}

size_t FormatNull(UnboundedBuffer *reply) {
  if (!reply) return 0;

  size_t oldSize = reply->ReadableSize();
  reply->PushData("$-1" CRLF, 5);

  return reply->ReadableSize() - oldSize;
}
