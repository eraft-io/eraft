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

#ifndef ERAFTKV_SERVER_H_
#define ERAFTKV_SERVER_H_

#include <cstdint>
#include <string>

#include "estatus.h"
#include "raft_server.h"

/**
 * @brief
 *
 */
struct ERaftKvServerOptions {
  std::string svr_version;
  std::string svr_addr;
  std::string kv_db_path;
  std::string log_db_path;

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
}

class ERaftKvServer : public grpc::EraftKv::Service {

  /**
   * @brief Construct a new ERaftKvServer object
   *
   * @param config
   */
  ERaftKvServer(ERaftKvServerOptions config) {
    this.options_ = config;
    // init raft lib
    RaftConfig raft_config;
    raft_config.net_impl = new GRpcNetworkImpl();
    raft_config.store_impl = new RocksDBStorageImpl();
    raft_config.log_impl = new RocksDBLogStorageImpl();
    raft_context_ = new RaftServer(raft_config);
  };


  /**
   * @brief
   *
   * @param req
   * @param resp
   * @return grpc::Status
   */
  grpc::Status RequestVote(RequestVoteReq* req, RequestVoteResp* resp){
      //
      // call raft_context_->HandleRequestVoteReq()
      //
  };

  /**
   * @brief
   *
   * @param req
   * @param resp
   * @return grpc::Status
   */
  grpc::Status AppendEntries(AppendEntriesReq* req, RequestVoteResp* resp){
      // 1.call raft_context_->HandleAppendEntriesReq()
  };

  /**
   * @brief
   *
   * @param req
   * @param resp
   * @return grpc::Status
   */
  grpc::Status Snapshot(SnapshotReq* req, SnapshotResp* resp){
      //  raftcore
      // 1.call raft_context_->HandleSnapshotReq();
  };

  /**
   * @brief
   *
   * @param req
   * @param resp
   * @return grpc::Status
   */
  grpc::Status ProcessRWOperation(ClientOperationReq*  req,
                                  ClientOperationResp* resp){
      // 1. req into log entry
      // 2. call raft_context_->ProposeEntry()
      // 3. wait commit
  };

  /**
   * @brief
   *
   * @return grpc::Status
   */
  grpc::Status ClusterConfigChange(ClusterConfigChangeReq,
                                   ClusterConfigChangeResp) {
    return EStatus::NotSupport();
  }

  /**
   * @brief
   *
   * @param interval
   * @return EStatus
   */
  EStatus InitTicker(int interval){
      // 1.set up raft_context_->RunCycle() run interval with periodic_caller_
  };

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus BuildAndRunRpcServer() {
    // set up rpc
    ERaftKvServer service;
    grpc::EnableDefaultHealthCheckService(true);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(this->options_.svr_addr,
                             grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    server->Wait();
  };

 private:
  /**
   * @brief
   *
   */
  RaftServer* raft_context_;

  /**
   * @brief
   *
   */
  PeriodicCaller* periodic_caller_;

  /**
   * @brief
   *
   */
  ERaftKvServerOptions options_;
};


#endif