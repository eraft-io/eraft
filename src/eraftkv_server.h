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
 * @file eraftkv_server.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <grpcpp/grpcpp.h>
#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <string>

#include "eraftkv.grpc.pb.h"
#include "eraftkv.pb.h"
#include "estatus.h"
#include "grpc_network_impl.h"
#include "raft_server.h"
#include "rocksdb_storage_impl.h"
#include "util.h"

using eraftkv::ERaftKv;
using grpc::ServerContext;
using grpc::Status;


enum ServerRoleEnum { DataServer, MetaServer };

/**
 * @brief
 *
 */
struct ERaftKvServerOptions {
  int64_t     svr_id;
  uint8_t     svr_role;
  std::string svr_version;
  std::string svr_addr;
  std::string peer_addrs;
  std::string monitor_addrs;
  std::string kv_db_path;
  std::string log_db_path;
  std::string snap_db_path;

  int64_t tick_interval;
  int64_t request_timeout;
  int64_t election_timeout;

  int64_t response_timeout;

  int64_t ae_max_count;
  int64_t ae_max_size;

  int64_t snap_max_count;
  int64_t snap_max_size;

  int64_t grpc_max_recv_msg_size;
  int64_t grpc_max_send_msg_size;
};

class ERaftKvServer : public eraftkv::ERaftKv::Service {

 public:
  /**
   * @brief Construct a new ERaftKvServer object
   *
   * @param config
   */
  ERaftKvServer(ERaftKvServerOptions option) : options_(option), op_sign(1) {
    // init raft lib
    RaftConfig raft_config;
    raft_config.id = options_.svr_id;
    auto    peers = StringUtil::Split(options_.peer_addrs, ',');
    int64_t count = 0;
    for (auto peer : peers) {
      raft_config.peer_address_map[count] = peer;
      count++;
    }
    raft_config.snap_path = options_.snap_db_path;
    options_.svr_addr = raft_config.peer_address_map[options_.svr_id];
    GRpcNetworkImpl* net_rpc = new GRpcNetworkImpl();
    net_rpc->InitPeerNodeConnections(raft_config.peer_address_map);
    RocksDBSingleLogStorageImpl* log_db =
        new RocksDBSingleLogStorageImpl(options_.log_db_path);
    RocksDBStorageImpl* kv_db = new RocksDBStorageImpl(options_.kv_db_path);
    raft_context_ =
        RaftServer::RunMainLoop(raft_config, log_db, kv_db, net_rpc);

    // put_counter = new prometheus::Family<prometheus::Counter>(std::move(ct));
  }

  ERaftKvServer() {}

  /**
   * @brief
   *
   * @param req
   * @param resp
   * @return grpc::Status
   */
  Status RequestVote(ServerContext*                 context,
                     const eraftkv::RequestVoteReq* req,
                     eraftkv::RequestVoteResp*      resp);

  /**
   * @brief
   *
   * @param req
   * @param resp
   * @return grpc::Status
   */
  Status AppendEntries(ServerContext*                   context,
                       const eraftkv::AppendEntriesReq* req,
                       eraftkv::AppendEntriesResp*      resp);
  /**
   * @brief
   *
   * @param req
   * @param resp
   * @return grpc::Status
   */
  Status Snapshot(ServerContext*              context,
                  const eraftkv::SnapshotReq* req,
                  eraftkv::SnapshotResp*      resp);

  /**
   * @brief
   *
   * @param req
   * @param resp
   * @return grpc::Status
   */
  Status ProcessRWOperation(ServerContext*                     context,
                            const eraftkv::ClientOperationReq* req,
                            eraftkv::ClientOperationResp*      resp);

  /**
   * @brief
   *
   * @return grpc::Status
   */
  Status ClusterConfigChange(ServerContext*                         context,
                             const eraftkv::ClusterConfigChangeReq* req,
                             eraftkv::ClusterConfigChangeResp*      resp);


  Status PutSSTFile(ServerContext*                               context,
                    grpc::ServerReader<eraftkv::SSTFileContent>* reader,
                    eraftkv::SSTFileId*                          fileId);

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus BuildAndRunRpcServer();

  /**
   * @brief
   *
   * @param log_idx
   * @return EStatus
   */
  EStatus TakeSnapshot(int64_t log_idx);

  /**
   * @brief
   *
   */
  ERaftKvServerOptions options_;

  std::shared_ptr<prometheus::Registry> regis;

  static std::map<int, std::condition_variable*> ready_cond_vars_;

  static std::mutex ready_mutex_;

  static bool is_ok_;

  prometheus::Family<prometheus::Counter>* put_counter;

 private:
  /**
   * @brief
   *
   */
  static RaftServer* raft_context_;


  int op_sign;
};
