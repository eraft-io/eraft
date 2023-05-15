/**
 * @file eraftkv_server.cc
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-04-01
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "eraftkv_server.h"

#include <grpcpp/grpcpp.h>

/**
 * @brief
 *
 * @param req
 * @param resp
 * @return grpc::Status
 */
grpc::Status ERaftKvServer::RequestVote(ServerContext*                 context,
                                        const eraftkv::RequestVoteReq* req,
                                        eraftkv::RequestVoteResp*      resp) {
  this->raft_context_->HandleRequestVoteReq(nullptr, req, resp);
  return grpc::Status::OK;
}

/**
 * @brief
 *
 * @param req
 * @param resp
 * @return grpc::Status
 */
grpc::Status ERaftKvServer::AppendEntries(ServerContext* context,
                                          const eraftkv::AppendEntriesReq* req,
                                          eraftkv::AppendEntriesResp* resp) {
  this->raft_context_->HandleAppendEntriesReq(nullptr, req, resp);
  return grpc::Status::OK;
}

/**
 * @brief
 *
 * @param req
 * @param resp
 * @return grpc::Status
 */
grpc::Status ERaftKvServer::Snapshot(ServerContext*              context,
                                     const eraftkv::SnapshotReq* req,
                                     eraftkv::SnapshotResp*      resp) {
  return grpc::Status::OK;
}

/**
 * @brief
 *
 * @param req
 * @param resp
 * @return grpc::Status
 */
grpc::Status ERaftKvServer::ProcessRWOperation(
    ServerContext*                     context,
    const eraftkv::ClientOperationReq* req,
    eraftkv::ClientOperationResp*      resp) {
  int64_t log_index;
  int64_t log_term;
  bool    success;
  this->raft_context_->Propose(
      req->SerializeAsString(), &log_index, &log_term, &success);
  return grpc::Status::OK;
}

/**
 * @brief
 *
 * @return grpc::Status
 */
grpc::Status ERaftKvServer::ClusterConfigChange(
    ServerContext*                         context,
    const eraftkv::ClusterConfigChangeReq* req,
    eraftkv::ClusterConfigChangeResp*      resp) {
  return grpc::Status::OK;
}

EStatus ERaftKvServer::BuildAndRunRpcServer() {
  // set up rpc
  ERaftKvServer service;
  grpc::EnableDefaultHealthCheckService(true);
  grpc::ServerBuilder builder;
  builder.AddListeningPort(this->options_.svr_addr,
                           grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  server->Wait();
  return EStatus::kOk;
}