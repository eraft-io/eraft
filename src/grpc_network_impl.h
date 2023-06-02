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
 * @file grpc_network_impl.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef GRPC_NETWORK_IMPL_H_
#define GRPC_NETWORK_IMPL_H_
#include <grpcpp/grpcpp.h>

#include "eraftkv.grpc.pb.h"
#include "eraftkv.pb.h"
#include "raft_server.h"

using eraftkv::ERaftKv;

class GRpcNetworkImpl : public Network {

 public:
  /**
   * @brief
   *
   * @param raft
   * @param target_node
   * @param req
   * @return EStatus
   */
  EStatus SendRequestVote(RaftServer*              raft,
                          RaftNode*                target_node,
                          eraftkv::RequestVoteReq* req);

  /**
   * @brief
   *
   * @param raft
   * @param target_node
   * @param req
   * @return EStatus
   */
  EStatus SendAppendEntries(RaftServer*                raft,
                            RaftNode*                  target_node,
                            eraftkv::AppendEntriesReq* req);

  /**
   * @brief
   *
   * @param raft
   * @param target_node
   * @param req
   * @return EStatus
   */
  EStatus SendSnapshot(RaftServer*           raft,
                       RaftNode*             target_node,
                       eraftkv::SnapshotReq* req);

  /**
   * @brief
   *
   * @param peers_address
   * @return EStatus
   */
  EStatus InitPeerNodeConnections(std::map<int64_t, std::string> peers_address);

  /**
   * @brief Get the Peer Node Connection object
   *
   * @param node_id
   * @return std::unique_ptr<EraftKv::Stub>
   */
  ERaftKv::Stub* GetPeerNodeConnection(int64_t node_id);

  /**
   * @brief 
   * 
   * @param peer_id 
   * @param addr 
   * @return EStatus 
   */
  EStatus InsertPeerNodeConnection(int64_t peer_id, std::string addr);

 private:
  /**
   * @brief
   *
   */
  std::map<int64_t, std::unique_ptr<ERaftKv::Stub>> peer_node_connections_;
};

#endif  // SRC_GRPC_NETWORK_IMPL_H_
