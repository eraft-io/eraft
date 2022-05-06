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

#ifndef ERAFT_PROTO_PARSER_H_
#define ERAFT_PROTO_PARSER_H_

#include <cctype>
#include <string>
#include <vector>

enum class ParseResult : int8_t {
  ok,
  wait,
  error,
};

ParseResult GetIntUntilCRLF(const char*& ptr, std::size_t nBytes, int& val);

class ProtoParser {
 public:
  void Reset();
  ParseResult ParseRequest(const char*& ptr, const char* end);
  const std::vector<std::string>& GetParams() const { return params_; }
  void SetParams(std::vector<std::string> p) { params_ = std::move(p); }
  bool IsInitialState() const { return multi_ == -1; }

 private:
  // ptr 是 char* 类型的引用
  ParseResult _ParseMulti(const char*& ptr, const char* end, int& result);
  ParseResult _ParseStrlist(const char*& ptr, const char* end,
                            std::vector<std::string>& results);
  ParseResult _ParseStr(const char*& ptr, const char* end, std::string& result);
  ParseResult _ParseStrval(const char*& ptr, const char* end,
                           std::string& result);
  ParseResult _ParseStrlen(const char*& ptr, const char* end, int& result);

  int multi_ = -1;
  int paramLen_ = -1;

  size_t numOfParam_ = 0;
  std::vector<std::string> params_;
};

#endif