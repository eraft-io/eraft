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

#include "raft_server.h"

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
                          eraftkv::RequestVoteReq* req) {
    // 1.send request vote with grpc message to target_node
    // 2.call raft->HandleRequestVoteResp();
  }


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
                            eraftkv::AppendEntriesReq* req) {}

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
                       eraftkv::SnapshotReq* req) {}


  /**
   * @brief
   *
   * @param peers_address
   * @return EStatus
   */
  EStatus InitPeerNodeConnections(
      std::map<std::string, std::string> peers_address) {
    // parse peers address build connection stubs to peer
  }

  /**
   * @brief Get the Peer Node Connection object
   *
   * @param node_id
   * @return std::unique_ptr<EraftKv::Stub>
   */
  std::unique_ptr<EraftKv::Stub> GetPeerNodeConnection(std::string node_id) {
    // get peer grpc connection stub form peer node connections map
  }

 private:
  /**
   * @brief
   *
   */
  std::map<std::string, std::unique_ptr<EraftKv::Stub>> peer_node_connections_;
};
