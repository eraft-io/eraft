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
#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

#include "eraftkv_server.h"
#include "raft_server.h"

DEFINE_int32(svr_id, 0, "server id");
DEFINE_string(kv_db_path, "", "kv rocksdb path");
DEFINE_string(log_db_path, "", "log rocksdb path");
DEFINE_string(peer_addrs, "", "peer address");
DEFINE_string(monitor_addrs, "", "monitor address");

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
  options_.monitor_addrs = FLAGS_monitor_addrs;

  ERaftKvServer server(options_);

  prometheus::Exposer exposer(options_.monitor_addrs);
  auto                registry = std::make_shared<prometheus::Registry>();
  exposer.RegisterCollectable(registry);
  server.regis = registry;

  server.BuildAndRunRpcServer();
  return 0;
}
