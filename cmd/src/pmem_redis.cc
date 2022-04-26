//
//  TiRedis.cc
//

#include <cmd/pmem_redis.h>
#include <network/client.h>
#include <network/command.h>
#include <network/config.h>
#include <network/executor.h>
#include <network/raft_config.h>
#include <network/raft_stack.h>
#include <network/socket.h>
#include <spdlog/common.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <iostream>

PMemRedis::PMemRedis() : port_(0) {}

PMemRedis::~PMemRedis() {}

network::RaftStack* PMemRedis::instance_ = nullptr;

std::shared_ptr<StreamSocket> PMemRedis::_OnNewConnection(int connfd, int tag) {
  // new connection comming
  SocketAddr peer;
  Socket::GetPeerAddr(connfd, peer);

  auto cli(std::make_shared<Client>());
  if (!cli->Init(connfd, peer)) cli.reset();
  return cli;
}

void PMemRedis::InitSpdLogger() {
  auto console = spdlog::stdout_logger_nt("console");
  spdlog::set_default_logger(console);
  spdlog::set_level(spdlog::level::debug);
  spdlog::set_pattern("[source %s] [function %!] [line %#] %v");
}

bool PMemRedis::_Init() {
  // init logger
  InitSpdLogger();

  // init server
  SocketAddr addr(g_config.listenAddr);
  std::vector<std::string> engine_params{g_config.dbPath};
  CommandTable::Init();
  Executor::Init(engine_params);
  if (!Server::TCPBind(addr, 1)) {
    return false;
  }
  std::cout << "server listen on: " << g_config.listenAddr << std::endl;

  // init raftstore
  std::shared_ptr<network::RaftConfig> raftConf =
      std::make_shared<network::RaftConfig>(g_config.listenAddr,
                                            g_config.dbPath, g_config.nodeId);

  network::RaftStack* stack = PMemRedis::MakeStackInstance(raftConf);
  stack->Start();
  SPDLOG_INFO("eraft start successful!");
  return true;
}

bool PMemRedis::_RunLogic() { return Server::_RunLogic(); }

bool PMemRedis::_Recycle() {
  // free resources
}

int main(int ac, char* av[]) {
  PMemRedis svr;

  if (!LoadServerConfig(std::string(av[1]).c_str(), g_config)) {
    std::cerr << "Load config file pmem_redis.toml failed!\n";
    return -2;
  }

  svr.MainLoop(false);

  return 0;
}
