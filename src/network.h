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

#pragma once

#include "estatus.h"
#include "raft_node.h"
#include "raft_server.h"

class RaftServer;

/**
 * @brief
 *
 */
class Network {
 public:
  /**
   * @brief Destroy the Network object
   *
   */
  virtual ~Network() {}

  /**
   * @brief
   *
   * @param raft
   * @param target_node
   * @param req
   * @return EStatus
   */
  virtual EStatus SendRequestVote(RaftServer*              raft,
                                  RaftNode*                target_node,
                                  eraftkv::RequestVoteReq* req) = 0;

  /**
   * @brief
   *
   * @param raft
   * @param target_node
   * @param req
   * @return EStatus
   */
  virtual EStatus SendAppendEntries(RaftServer*                raft,
                                    RaftNode*                  target_node,
                                    eraftkv::AppendEntriesReq* req) = 0;

  /**
   * @brief
   *
   * @param raft
   * @param target_node
   * @param req
   * @return EStatus
   */
  virtual EStatus SendSnapshot(RaftServer*           raft,
                               RaftNode*             target_node,
                               eraftkv::SnapshotReq* req) = 0;

  /**
   * @brief
   *
   * @param raft
   * @param raft_node
   * @param filename
   * @return EStatus
   */
  virtual EStatus SendFile(RaftServer*        raft,
                           RaftNode*          raft_node,
                           const std::string& filename) = 0;

  /**
   * @brief
   *
   * @param peers_address
   * @return EStatus
   */
  virtual EStatus InitPeerNodeConnections(
      std::map<int64_t, std::string> peers_address) = 0;

  /**
   * @brief
   *
   * @param peer_id
   * @param addr
   * @return EStatus
   */
  virtual EStatus InsertPeerNodeConnection(int64_t     peer_id,
                                           std::string addr) = 0;
};

/**
 * @brief
 *
 */
class Event {
 public:
  /**
   * @brief Destroy the Event object
   *
   */
  virtual ~Event() {}

  /**
   * @brief
   *
   * @param raft
   * @param state
   */
  virtual void RaftStateChangeEvent(RaftServer* raft, int state) = 0;

  /**
   * @brief
   *
   * @param raft
   * @param node
   * @param ety
   */
  virtual void RaftGroupMembershipChangeEvent(RaftServer*     raft,
                                              RaftNode*       node,
                                              eraftkv::Entry* ety) = 0;
};