// MIT License

// Copyright (c) 2023 ERaftGroup

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

/**
 * @file main.cc
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <gflags/gflags.h>
#include <rocksdb/db.h>
#include <spdlog/common.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <iostream>

#include "eraft/raft_server.h"
#include "eraftkv_server.h"
#include "httplib.h"

DEFINE_int32(svr_id, 0, "server id");
DEFINE_string(kv_db_path, "", "kv rocksdb path");
DEFINE_string(log_db_path, "", "log rocksdb path");
DEFINE_string(snap_db_path, "", "snapshot db path");
DEFINE_string(peer_addrs, "", "peer address");
DEFINE_string(log_file_path, "", "log file path");
DEFINE_int32(monitor_port, 18080, "monitor port");

static void RunHTTPServer(std::atomic<std::string*>* json_stat,
                          int32_t                    monitor_port) {
  httplib::Server svr;
  svr.Get("/collect_stats",
          [json_stat](const httplib::Request& req, httplib::Response& res) {
            ERaftKvServer::ReportStats();
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(*(*json_stat), "application/json");
          });
  svr.listen("0.0.0.0", monitor_port);
}

/**
 * @brief
 *
 * @param argc
 * @param argv (eg: eraftkv 0 /tmp/kv_db0 /tmp/log_db0 /tmp/snap_db0
 * 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090)
 * eraftkv [node id] [kv data path] [log data path] [snap db path] [meta server
 * addrs] [log_file_path]
 * @return int
 */
int main(int argc, char* argv[]) {
  gflags::SetUsageMessage("ERaftKDB");
  gflags::SetVersionString("1.0.0");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  ERaftKvServerOptions options_;
  options_.svr_role = ServerRoleEnum::DataServer;
  options_.svr_id = FLAGS_svr_id;
  options_.kv_db_path = FLAGS_kv_db_path;
  options_.log_db_path = FLAGS_log_db_path;
  options_.snap_db_path = FLAGS_snap_db_path;
  options_.peer_addrs = FLAGS_peer_addrs;
  std::string   log_file_path = FLAGS_log_file_path;
  ERaftKvServer server(options_, 0);

  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::debug);
  console_sink->set_pattern("[%H:%M:%S %z] [%@] %v");

  auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_st>(
      log_file_path, 23, 59);
  file_sink->set_level(spdlog::level::debug);
  file_sink->set_pattern("[%H:%M:%S %z] [%@] %v");

  spdlog::sinks_init_list sink_list = {file_sink, console_sink};

  spdlog::logger logger("multi_sink", sink_list.begin(), sink_list.end());
  logger.set_level(spdlog::level::debug);
  logger.warn("this should appear in both console and file");
  logger.info(
      "this message should not appear in the console, only in the file");

  spdlog::set_default_logger(std::make_shared<spdlog::logger>(
      "multi_sink", spdlog::sinks_init_list({console_sink, file_sink})));
  SPDLOG_INFO("eraftkv server start with peer_addrs " + options_.peer_addrs +
              " kv_db_path " + options_.kv_db_path);

  std::thread httpSvrThread(
      &RunHTTPServer, &ERaftKvServer::stat_json_str_, FLAGS_monitor_port);
  httpSvrThread.detach();

  server.BuildAndRunRpcServer();
  return 0;
}
