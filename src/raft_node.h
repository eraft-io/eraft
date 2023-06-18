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
 * @file raft_node.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include <cstdint>
#include <string>


/**
 * @brief
 *
 */
enum NodeStateEnum { Init, Voting, Running, Down, LostConnection };

/**
 * @brief
 *
 */
struct RaftNode {
  int64_t       id;
  NodeStateEnum node_state;
  int64_t       next_log_index;
  int64_t       match_log_index;
  std::string   address;

  RaftNode(int64_t       id_,
           NodeStateEnum node_state_,
           int64_t       next_log_index_,
           int64_t       match_log_index_,
           std::string   address_)
      : id(id_)
      , node_state(node_state_)
      , next_log_index(next_log_index_)
      , match_log_index(match_log_index_)
      , address(address_) {}
};
