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
  int64_t       id;
  NodeStateEnum node_state;
  int64_t       next_log_index;
  int64_t       match_log_index;
  std::string        address;

  RaftNode(int64_t       id_,
           NodeStateEnum node_state_,
           int64_t       next_log_index_,
           int64_t       match_log_index_, 
           std::string       address_)
      : id(id_)
      , node_state(node_state_)
      , next_log_index(next_log_index_)
      , match_log_index(match_log_index_)
      , address(address_) {}
};

#endif