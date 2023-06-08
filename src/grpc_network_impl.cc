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
#include "util.h"

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
  TraceLog("DEBUG: ", " send req vote to ", target_node->address);
  ERaftKv::Stub* stub_ = GetPeerNodeConnection(target_node->id);
  if (stub_ == nullptr) {
    return EStatus::kNotFound;
  }
  eraftkv::RequestVoteResp* resp = new eraftkv::RequestVoteResp;
  resp->set_request_term(0);
  resp->set_term(0);
  resp->set_leader_id(-1);
  ClientContext context;
  auto          status = stub_->RequestVote(&context, *req, resp);
  // 2.call raft->HandleRequestVoteResp();
  if (raft->HandleRequestVoteResp(target_node, req, resp) == EStatus::kOk) {
    return EStatus::kOk;
  } else {
    return EStatus::kNotFound;
  }
  delete resp;
  return EStatus::kOk;
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
  resp->set_term(0);
  resp->set_current_index(0);
  resp->set_conflict_index(0);
  resp->set_conflict_term(0);
  ClientContext context;
  auto          status = stub_->AppendEntries(&context, *req, resp);
  if (!status.ok()) {
    TraceLog("ERROR: ", " send append req to FAILED! ", target_node->address);
  }
  // 2.call raft->HandleAppendEntriesResp();
  if (raft->HandleAppendEntriesResp(target_node, req, resp) == EStatus::kOk) {
    return EStatus::kOk;
  } else {
    return EStatus::kNotFound;
  }
  delete resp;
  return EStatus::kOk;
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
  delete resp;
  return EStatus::kOk;
}


/**
 * @brief
 *
 * @param peers_address
 * @return EStatus
 */
EStatus GRpcNetworkImpl::InitPeerNodeConnections(
    std::map<int64_t, std::string> peers_address) {
  // parse peers address build connection stubs to peer
  for (auto itr : peers_address) {
    auto chan_ =
        grpc::CreateChannel(itr.second, grpc::InsecureChannelCredentials());
    auto stub_(ERaftKv::NewStub(chan_));
    this->peer_node_connections_[itr.first] = std::move(stub_);
    TraceLog("DEBUG: ", "init peer connection ", itr.second);
  }
  return EStatus::kOk;
}

EStatus GRpcNetworkImpl::InsertPeerNodeConnection(int64_t peer_id, std::string addr) {
  auto chan_ =
        grpc::CreateChannel(addr, grpc::InsecureChannelCredentials());
  auto stub_(ERaftKv::NewStub(chan_));
  this->peer_node_connections_[peer_id] = std::move(stub_);
  TraceLog("DEBUG: ", "insert peer connection to ", addr);
  return EStatus::kOk;
}

/**
 * @brief Get the Peer Node Connection object
 *
 * @param node_id
 * @return std::unique_ptr<EraftKv::Stub>
 */
ERaftKv::Stub* GRpcNetworkImpl::GetPeerNodeConnection(int64_t node_id) {
  // get peer grpc connection stub form peer node connections map
  if (this->peer_node_connections_.find(node_id) !=
      this->peer_node_connections_.end()) {
    return this->peer_node_connections_[node_id].get();
  }
  return nullptr;
}