// MIT License

// Copyright (c) 2021 Colin

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

#ifndef _ERAFT_LOGGER_H_
#define _ERAFT_LOGGER_H_

#include <stdint.h>
#include <time.h>

#include <fstream>
#include <iostream>

class Logger {
 public:
  enum LogLevel { kDebug, kInfo, kWarning, kError };

  enum LogTarget { kFile, kTerminal, kFileAndTerminal };

 public:
  Logger();
  Logger(LogTarget target, LogLevel level, const std::string& path);
  ~Logger();

  void DEBUG_NEW(const std::string& in, const std::string& file, uint64_t line,
                 const std::string& function);
  void DEBUG(const std::string& text);
  void INFO(const std::string& text);
  void WARNING(const std::string& text);
  void ERRORS(const std::string& text);

  static Logger* GetInstance() {
    if (instance_ == nullptr) {
      instance_ = new Logger(Logger::kTerminal, Logger::kDebug, "Log.log");
    }
    return instance_;
  }

 private:
  static Logger* instance_;

  std::ofstream outfile_;  // 将日志输出到文件的流对象
  LogTarget target_;       // 日志输出目标
  std::string path_;       // 日志文件路径
  LogLevel level_;         // 日志等级
  void Output(const std::string& text, LogLevel act_level);  // 输出行为
};

#endif  //_LOGGER_H_
