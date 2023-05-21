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
 * @file eraftmeta_server.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef SRC_ERAFTMETA_SERVER_H_
#define SRC_ERAFTMETA_SERVER_H_
#include <cstdint>
#include <string>

#include "estatus.h"
#include "raft_server.h"

/**
 * @brief
 *
 */
struct ERaftMetaServerOptions {

  /**
   * @brief server version
   *
   */
  std::string svr_version;

  /**
   * @brief server listen address
   *
   */
  std::string svr_addr;

  /**
   * @brief the path of kv rocksdb
   *
   */
  std::string kv_db_path;

  /**
   * @brief the path of log rocksdb
   *
   */
  std::string log_db_path;

  /**
   * @brief call raftcore clycle interval
   *
   */
  int64_t tick_interval;

  /**
   * @brief raft request timeout
   *
   */
  int64_t request_timeout;

  /**
   * @brief raft election timeout
   *
   */
  int64_t election_timeout;

  /**
   * @brief raft deal client request timeout
   *
   */
  int64_t response_timeout;

  /**
   * @brief append entries max count in one rpc
   *
   */
  int64_t ae_max_count;

  /**
   * @brief append entries max size in one rpc
   *
   */
  int64_t ae_max_size;

  /**
   * @brief snapshot max count in one rpc
   *
   */
  int64_t snap_max_count;

  /**
   * @brief snapshot max count in one rpc
   *
   */
  int64_t snap_max_size;

  /**
   * @brief grpc max recv msg size once
   *
   */
  int64_t grpc_max_recv_msg_size;

  /**
   * @brief grpc max send msg size once
   *
   */
  int64_t grpc_max_send_msg_size;
};

/**
 * @brief
 *
 */
class ERaftMetaServer : public grpc::EraftKv::Service {

 public:
  /**
   * @brief Construct a new ERaftKvServer object
   *  Initialize the meta server
   * @param config
   */
  ERaftKvServer(ERaftMetaServerOptions config) {
    RaftConfig raft_config;
    raft_config.net_impl = new GRpcNetworkImpl();
    raft_config.store_impl = new RocksDBStorageImpl();
    raft_config.log_impl = new RocksDBLogStorageImpl();
    raft_context_ = new RaftServer(raft_config)
  }

  /**
   * @brief  RequestVote sends a voterequest by candidates
   *
   * @return grpc::Status
   */
  grpc::Status RequestVote(RequestVoteReq, RequestVoteResp) {
    return grpc::Status::OK;
  }

  /**
   * @brief  AppendEntries sends an append rpc with new entries and
   * the current commit index to followers
   *
   * @return grpc::Status
   */
  grpc::Status AppendEntries(AppendEntriesReq, RequestVoteResp) {
    return grpc::Status::OK;
  }
  /**
   * @brief  Snapshot send a snaphost rpc with snapshot
   *
   * @return grpc::Status
   */
  grpc::Status Snapshot(SnapshotReq, SnapshotResp) {
    return grpc::Status::OK;
  }
  /**
   * @brief ProcessRWOperation recv read|write request
   *
   * @return grpc::Status
   */
  grpc::Status ProcessRWOperation(ClientOperationReq, ClientOperationResp) {
    return grpc::Status::OK;
  }

  grpc::Status ClusterConfigChange(ClusterConfigChangeReq,
                                   ClusterConfigChangeResp) {
    return grpc::Status::OK;
  }

  /**
   * @brief BuildAndRunRpcServer make new rpc server and run
   *
   * @return EStatus
   */
  EStatus BuildAndRunRpcServer() {
    return EStatus::kOk;
  }
  /**
   * @brief RunRaftCycle raft loop
   *
   * @return EStatus
   */
  EStatus RunRaftCycle() {
    return EStatus::kOk;
  }

 private:
  /**
   * @brief raft core impl context
   *
   */
  RaftServer* raft_context_;

  /**
   * @brief meta server options
   *
   */
  ERaftMetaServerOptions options_;
};


#endif  // SRC_ERAFTMETA_SERVER_H_
