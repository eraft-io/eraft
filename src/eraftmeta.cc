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

#include "eraftkv_server.h"
#include "raft_server.h"
/**
 * @brief
 *
 * @param argc
 * @param argv (eg: eraftmeta 0 /tmp/meta_db0 /tmp/log_db0
 * 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090)
 * @return int
 */
int main(int argc, char* argv[]) {
  ERaftKvServerOptions options_;
  options_.svr_role = ServerRoleEnum::MetaServer;
  options_.svr_id = stoi(std::string(argv[1]));
  options_.kv_db_path = std::string(argv[2]);
  options_.log_db_path = std::string(argv[3]);
  options_.peer_addrs = std::string(argv[4]);
  ERaftKvServer server(options_);
  server.BuildAndRunRpcServer();
  return 0;
}
