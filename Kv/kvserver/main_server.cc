// MIT License

// Copyright (c) 2021 eraft dev group

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

#include <Kv/config.h>
#include <Kv/raft_server.h>
#include <Kv/raft_store.h>
#include <Kv/server.h>
#include <Kv/ticker.h>
#include <spdlog/common.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>

// check version
int main(int argc, char* argv[]) {
  // make conf
  std::shared_ptr<kvserver::Config> conf = std::make_shared<kvserver::Config>(
      std::string(argv[1]), std::string(argv[2]),
      std::stoi(std::string(argv[3])));

  // init logger
  auto console = spdlog::stdout_logger_mt("console");
  spdlog::set_default_logger(console);
  spdlog::set_level(spdlog::level::debug);
  spdlog::set_pattern("[source %s] [function %!] [line %#] %v");

  conf->PrintConfigToConsole();

  // start raft store
  std::unique_ptr<kvserver::RaftStorage> storage(
      new kvserver::RaftStorage(conf));
  storage->Start();
  SPDLOG_LOGGER_INFO(console, "eraft start successful!");

  // start rpc service server
  kvserver::Server svr(conf->storeAddr_, storage.get());
  svr.RunLogic();

  return 0;
}
