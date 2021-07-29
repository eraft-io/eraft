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

#include <Logger/Logger.h>

#include <string>

Logger* Logger::instance_ = nullptr;

std::string currTime() {
  char tmp[64];
  time_t ptime;
  time(&ptime);
  strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&ptime));
  return tmp;
}

Logger::Logger() {
  target_ = kTerminal;
  level_ = kDebug;
  std::cout << currTime() << " : "
            << "=== Start logging ===" << std::endl;
}

Logger::Logger(LogTarget target, LogLevel level, const std::string& path) {
  target_ = target;
  path_ = path;
  level_ = level;

  std::string strContent = currTime() + " : " + "=== Start logging ===\n";
  if (target != kTerminal) {
    outfile_.open(path, std::ios::out | std::ios::app);
    outfile_ << strContent;
  }
  if (target != kFile) {
    std::cout << strContent;
  }
}

Logger::~Logger() {
  std::string strContent = currTime() + " : " + "=== End logging ===\r\n";
  if (outfile_.is_open()) {
    outfile_ << strContent;
  }
  outfile_.flush();
  outfile_.close();
}

void Logger::DEBUG(const std::string& text) { Output(text, kDebug); }

void Logger::DEBUG_NEW(const std::string& in, const std::string& file,
                       uint64_t line, const std::string& function) {
  std::string text;
  text.append(" [ ");
  text.append(file);
  text.append(":");
  text.append(std::to_string(line));
  text.append(" ]");
  text.append(" ");
  text.append(function);
  text.append(" [ ");
  text.append(std::string(in));
  text.append(" ]");
  output(text, kDebug);
}

void Logger::INFO(const std::string& text) { Output(text, kInfo); }

void Logger::WARNING(const std::string& text) { Output(text, kWarning); }

void Logger::ERRORS(const std::string& text) { Output(text, kError); }

void Logger::Output(const std::string& text, LogLevel act_level) {
  std::string prefix;
  if (act_level == kDebug)
    prefix = "[DEBUG] ";
  else if (act_level == kInfo)
    prefix = "[INFO] ";
  else if (act_level == kWarning)
    prefix = "[WARNING] ";
  else if (act_level == kError)
    prefix = "[ERROR] ";
  std::string outputContent = prefix + " " + currTime() + " : " + text + "\n";
  if (level_ <= act_level && target_ != kFile) {
    std::cout << outputContent;
  }
  if (target_ != kTerminal) outfile_ << outputContent;
  outfile_.flush();
}
