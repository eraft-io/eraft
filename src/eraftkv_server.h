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

#ifndef SRC_ERAFTKV_SERVER_H_
#define SRC_ERAFTKV_SERVER_H_

#include <grpcpp/grpcpp.h>

#include <cstdint>
#include <memory>
#include <string>

#include "eraftkv.grpc.pb.h"
#include "eraftkv.pb.h"
#include "estatus.h"
#include "raft_server.h"

using eraftkv::ERaftKv;
using grpc::ServerContext;
using grpc::Status;

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
};

class ERaftKvServer : public eraftkv::ERaftKv::Service {

 public:
  /**
   * @brief Construct a new ERaftKvServer object
   *
   * @param config
   */
  ERaftKvServer(ERaftKvServerOptions option) : options_(option) {
    // init raft lib
    RaftConfig raft_config;
    this->raft_context_ = RaftServer::RunMainLoop(raft_config);
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
  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus BuildAndRunRpcServer();

  /**
   * @brief
   *
   */
  ERaftKvServerOptions options_;

 private:
  /**
   * @brief
   *
   */
  RaftServer* raft_context_;
};


#endif  // SRC_ERAFTKV_SERVER_H_
