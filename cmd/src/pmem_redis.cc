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

#include <cmd/pmem_redis.h>

#include <iostream>


PMemRedis::PMemRedis() : port_(0) {}

PMemRedis::~PMemRedis() { }


std::shared_ptr<StreamSocket> PMemRedis::_OnNewConnection(int connfd, int tag) {
  // new connection comming
  SocketAddr peer;
  Socket::GetPeerAddr(connfd, peer);

  auto cli(std::make_shared<Client>());
  if (!cli->Init(connfd, peer)) cli.reset();
  return cli;
}

void PMemRedis::InitSpdLogger() {
  auto console = spdlog::stdout_logger_mt("console");
  spdlog::set_default_logger(console);
  spdlog::set_level(spdlog::level::trace);
  spdlog::set_pattern("[source %s] [function %!] [line %#] %v");
}

bool PMemRedis::_Init() {
  // init logger
  InitSpdLogger();

  // init server
  SocketAddr addr(g_config.listenAddr);
  CommandTable::Init();
  if (!Server::TCPBind(addr, 1)) {
    return false;
  }
  std::cout << "server listen on: " << g_config.listenAddr << std::endl;

  // init raftstore
  std::shared_ptr<network::RaftConfig> raftConf =
      std::make_shared<network::RaftConfig>(g_config.listenAddr,
                                            g_config.dbPath, g_config.nodeId);

  raftStack_ = std::make_shared<network::RaftStack>(raftConf);
  if (raftStack_->Start()) {
    SPDLOG_INFO("raft stack start successful!");
  }
  return true;
}

bool PMemRedis::_RunLogic() { return Server::_RunLogic(); }

bool PMemRedis::_Recycle() {
  // free resources
}

int main(int ac, char* av[]) {
  if (!LoadServerConfig(std::string(av[1]).c_str(), g_config)) {
    std::cerr << "Load config file pmem_redis.toml failed!\n";
    return -2;
  }

  PMemRedis::GetInstance()->MainLoop(false);

  return 0;
}
