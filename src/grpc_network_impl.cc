/**
 * @file grpc_network_impl.cc
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-04-08
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "grpc_network_impl.h"

#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <iostream>
#include <memory>

#include "eraftkv.grpc.pb.h"
#include "eraftkv.pb.h"
#include "raft_server.h"

using eraftkv::ERaftKv;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
/**
 * @brief
 *
 * @param raft
 * @param target_node
 * @param req
 * @return EStatus
 */
EStatus GRpcNetworkImpl::SendRequestVote(RaftServer*              raft,
                                         RaftNode*                target_node,
                                         eraftkv::RequestVoteReq* req) {
  // 1.send request vote with grpc message to target_node
  ERaftKv::Stub* stub_ = GetPeerNodeConnection(target_node->id);
  if (stub_ == nullptr) {
    return EStatus::kNotFound;
  }
  eraftkv::RequestVoteResp* resp = new eraftkv::RequestVoteResp;
  ClientContext             context;
  auto                      status = stub_->RequestVote(&context, *req, resp);
  // 2.call raft->HandleRequestVoteResp();
  if (raft->HandleRequestVoteResp(target_node, req, resp) == EStatus::kOk) {
    return EStatus::kOk;
  } else {
    return EStatus::kNotFound;
  }
  // TO DO delete RequestVoteResp
}


/**
 * @brief
 *
 * @param raft
 * @param target_node
 * @param req
 * @return EStatus
 */
EStatus GRpcNetworkImpl::SendAppendEntries(RaftServer* raft,
                                           RaftNode*   target_node,
                                           eraftkv::AppendEntriesReq* req) {
  // 1.send entries with grpc message to target_node
  ERaftKv::Stub* stub_ = GetPeerNodeConnection(target_node->id);
  if (stub_ == nullptr) {
    return EStatus::kNotFound;
  }
  eraftkv::AppendEntriesResp* resp = new eraftkv::AppendEntriesResp;
  ClientContext               context;
  auto status = stub_->AppendEntries(&context, *req, resp);
  // 2.call raft->HandleAppendEntriesResp();
  if (raft->HandleAppendEntriesResp(target_node, req, resp) == EStatus::kOk) {
    return EStatus::kOk;
  } else {
    return EStatus::kNotFound;
  }
  // TO DO delete AppendEntriesResp
}

/**
 * @brief
 *
 * @param raft
 * @param target_node
 * @param req
 * @return EStatus
 */
EStatus GRpcNetworkImpl::SendSnapshot(RaftServer*           raft,
                                      RaftNode*             target_node,
                                      eraftkv::SnapshotReq* req) {
  // 1.send snapshot to the target_node
  ERaftKv::Stub* stub_ = GetPeerNodeConnection(target_node->id);
  if (stub_ == nullptr) {
    return EStatus::kNotFound;
  }
  eraftkv::SnapshotResp* resp = new eraftkv::SnapshotResp;
  ClientContext          context;
  auto                   status = stub_->Snapshot(&context, *req, resp);
  // 2.call raft->HandleSnapshotResp();
  if (raft->HandleSnapshotResp(target_node, resp) == EStatus::kOk) {
    return EStatus::kOk;
  } else {
    return EStatus::kNotFound;
  }
  // TO DO delete SnapshotResp
}


/**
 * @brief
 *
 * @param peers_address
 * @return EStatus
 */
EStatus GRpcNetworkImpl::InitPeerNodeConnections(
    std::map<int, std::string> peers_address) {
  // parse peers address build connection stubs to peer
  for (auto itr : peers_address) {
    auto chan_ =
        grpc::CreateChannel(itr.second, grpc::InsecureChannelCredentials());
    auto stub_(ERaftKv::NewStub(chan_));
    this->peer_node_connections_[itr.first] = std::move(stub_);
  }
  return EStatus::kOk;
}

/**
 * @brief Get the Peer Node Connection object
 *
 * @param node_id
 * @return std::unique_ptr<EraftKv::Stub>
 */
ERaftKv::Stub* GRpcNetworkImpl::GetPeerNodeConnection(int node_id) {
  // get peer grpc connection stub form peer node connections map
  if (this->peer_node_connections_.find(node_id) !=
      this->peer_node_connections_.end()) {
    return this->peer_node_connections_[node_id].get();
  }
  return nullptr;
}