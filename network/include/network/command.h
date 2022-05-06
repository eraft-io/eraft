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

#ifndef ERAFT_COMMAND_H_
#define ERAFT_COMMAND_H_

#include <network/common.h>

#include <map>
#include <string>
#include <vector>

enum CommandAttr {
  Attr_read = 0x1,
  Attr_write = 0x1 << 1,
};

class UnboundedBuffer;
using CommandHandler = Error(const std::vector<std::string> &params,
                             UnboundedBuffer *reply);

CommandHandler set;
CommandHandler get;
CommandHandler del;
CommandHandler scan;
CommandHandler info;
CommandHandler pushraftmsg;

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

#endif
