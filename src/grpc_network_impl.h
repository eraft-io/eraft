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

 private:
  /**
   * @brief
   *
   */
  std::map<int64_t, std::unique_ptr<ERaftKv::Stub>> peer_node_connections_;
};

#endif // SRC_GRPC_NETWORK_IMPL_H_
