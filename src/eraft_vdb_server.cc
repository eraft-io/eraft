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

#include <gflags/gflags.h>
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

DEFINE_string(addr, "172.18.0.6:12306", "server address");
DEFINE_string(metasvr_addrs,
              "172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090",
              "eraftmeta server addrs");
DEFINE_string(logfile_path, "/eraft/logs/eraftkdb.log", "the path of log file");

/**
 * @brief
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char *argv[]) {

  gflags::SetUsageMessage("ERaftKDB");
  gflags::SetVersionString("1.0.0");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  std::string    addr = FLAGS_addr;
  std::string    meta_svr_addrs = FLAGS_metasvr_addrs;
  std::string    log_file_path = FLAGS_logfile_path;
  ERaftVdbServer svr(addr, meta_svr_addrs);

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

  SPDLOG_INFO("eraftkdb server start with addr " + addr + " meta_svr_addrs " +
              meta_svr_addrs);

  svr.MainLoop(false);
  return 0;
}
