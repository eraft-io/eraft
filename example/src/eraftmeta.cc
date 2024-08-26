/**
 * @file eraftmeta.cc
 * @author jay_jieliu@outlook.com
 * @brief
 * @version 0.1
 * @date 2023-06-26
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <gflags/gflags.h>

#include "eraft/raft_server.h"
#include "eraftkv_server.h"
#include "httplib.h"

DEFINE_int32(svr_id, 0, "server id");
DEFINE_string(kv_db_path, "", "kv rocksdb path");
DEFINE_string(log_db_path, "", "log rocksdb path");
DEFINE_string(peer_addrs, "", "peer address");
DEFINE_int32(monitor_port, 19080, "monitor port");

static void RunHTTPServer(std::atomic<std::string*>* json_stat,
                          int32_t                    monitor_port) {
  httplib::Server svr;
  svr.Get("/v1_cluster_stats",
          [json_stat](const httplib::Request& req, httplib::Response& res) {
            ERaftKvServer::UpdateMetaStats();
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_content(*(*json_stat), "application/json");
          });
  svr.listen("0.0.0.0", monitor_port);
}


/**
 * @brief
 *
 * @param argc
 * @param argv (eg: eraftmeta 0 /tmp/meta_db0 /tmp/log_db0
 * 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090)
 * @return int
 */
int main(int argc, char* argv[]) {
  gflags::SetUsageMessage("ERaftKDB");
  gflags::SetVersionString("1.0.0");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  ERaftKvServerOptions options_;
  options_.svr_role = ServerRoleEnum::MetaServer;
  options_.svr_id = FLAGS_svr_id;
  options_.kv_db_path = FLAGS_kv_db_path;
  options_.log_db_path = FLAGS_log_db_path;
  options_.peer_addrs = FLAGS_peer_addrs;

  ERaftKvServer server(options_, 1);
  std::thread   httpSvrThread(&RunHTTPServer,
                            &ERaftKvServer::cluster_stats_json_str_,
                            FLAGS_monitor_port);
  httpSvrThread.detach();

  server.BuildAndRunRpcServer();
  return 0;
}
