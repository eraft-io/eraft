/**
 * @file raft_node.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef RAFT_NODE_H_
#define RAFT_NODE_H_

#include <cstdint>
#include <string>

/**
 * @brief
 *
 */
enum NodeStateEnum { Init, Voting, Running, Down };

/**
 * @brief
 *
 */
struct RaftNode {
  std::string   id;
  int64_t       next_log_index;
  int64_t       match_log_index;
  NodeStateEnum node_state;
};

#endif