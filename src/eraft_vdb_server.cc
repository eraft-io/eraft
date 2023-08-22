/**
 * @file eraft_vdb_server.cc
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-06-17
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "eraft_vdb_server.h"

#include <spdlog/common.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include "client.h"
#include "server.h"
#include "socket.h"

ERaftVdbServer::ERaftVdbServer(std::string addr, std::string kv_svr_addrs)
    : port_(0), addr_(addr), kv_svr_addrs_(kv_svr_addrs) {}

ERaftVdbServer::~ERaftVdbServer() {}

std::shared_ptr<StreamSocket> ERaftVdbServer::_OnNewConnection(int connfd,
                                                               int tag) {
  SocketAddr peer;
  Socket::GetPeerAddr(connfd, peer);

  auto cli(std::make_shared<Client>(kv_svr_addrs_));
  if (!cli->Init(connfd, peer))
    cli.reset();
  return cli;
}

bool ERaftVdbServer::_Init() {
  SocketAddr addr(addr_);
  if (!Server::TCPBind(addr, 1)) {
    return false;
  }
  return true;
}

bool ERaftVdbServer::_RunLogic() {
  return Server::_RunLogic();
}

bool ERaftVdbServer::_Recycle() {
  return true;
}

/**
 * @brief
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char *argv[]) {
  std::string    addr = std::string(argv[1]);
  std::string    kv_svr_addrs = std::string(argv[2]);
  std::string    log_file_path = std::string(argv[3]);
  ERaftVdbServer svr(addr, kv_svr_addrs);

  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::trace);
  console_sink->set_pattern("[%H:%M:%S %z] [%@] %v");

  auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_st>(
      log_file_path, 23, 59);
  file_sink->set_level(spdlog::level::trace);
  file_sink->set_pattern("[%H:%M:%S %z] [%@] %v");

  spdlog::sinks_init_list sink_list = {file_sink, console_sink};

  spdlog::logger logger("multi_sink", sink_list.begin(), sink_list.end());
  logger.set_level(spdlog::level::trace);
  logger.warn("this should appear in both console and file");
  logger.info(
      "this message should not appear in the console, only in the file");

  spdlog::set_default_logger(std::make_shared<spdlog::logger>(
      "multi_sink", spdlog::sinks_init_list({console_sink, file_sink})));

  SPDLOG_INFO("eraftkdb server start with addr " + addr + " kv_svr_addrs " +
              kv_svr_addrs);

  svr.MainLoop(false);
  return 0;
}
