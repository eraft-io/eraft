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
#ifndef RAFT_CONFIG_H_
#define RAFT_CONFIG_H_

#include <map>
#include <string>
#include <vector>

#include "network.h"
#include "raft_log.h"
#include "raft_node.h"
#include "storage.h"
/**
 * @brief
 *
 */
struct RaftConfig {
  int64_t                            id;
  std::map<int64_t, std::string> peer_address_map;
  int64_t                            election_tick;
  int64_t                            heartbeat_tick;
  int64_t                            applied;
  bool                               prevote;
  Network*                           net_impl;
  Storage*                           store_impl;
  LogStore*                          log_impl;
};

#endif