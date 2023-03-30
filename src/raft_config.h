/**
 * @file raft_config.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "raft_server.h"

/**
 * @brief
 *
 */
struct RaftConfig {
  int64_t                               id;
  std::vector<std::string, std::string> peer_address_map;
  int64_t                               election_tick;
  int64_t                               heartbeat_tick;
  int64_t                               applied;
  bool                                  prevote;
  Network*                              net_impl;
  Storage*                              store_impl;
  LogStore*                             log_impl;
};
