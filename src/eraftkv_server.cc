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
#include "util.h"

#include <grpcpp/grpcpp.h>

RaftServer* ERaftKvServer::raft_context_ = nullptr;

std::map<int, std::condition_variable*> ERaftKvServer::ready_cond_vars_;

std::mutex ERaftKvServer::ready_mutex_;

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
  if(raft_context_->HandleRequestVoteReq(nullptr, req, resp) == EStatus::kOk) {
    return grpc::Status::OK;

  } else {
     return grpc::Status::CANCELLED;
  }
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
    if(raft_context_->HandleAppendEntriesReq(nullptr, req, resp) == EStatus::kOk) {
    return grpc::Status::OK;

  } else {
     return grpc::Status::CANCELLED;
  }
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
  TraceLog("DEBUG: ", " recv rw op with ts ", req->op_timestamp());
  for(auto kv_op: req->kvs()) {
    if(kv_op.op_type() == eraftkv::ClientOpType::Put) {
      std::mutex map_mutex_;
      {
        std::condition_variable* new_var = new std::condition_variable();
        std::lock_guard<std::mutex> lg(map_mutex_);
        ERaftKvServer::ready_cond_vars_[op_count_] = new_var;
        kv_op.set_op_count(op_count_);
      }
      raft_context_->Propose(
      kv_op.SerializeAsString(), &log_index, &log_term, &success);
      {
        std::unique_lock<std::mutex> ul(ERaftKvServer::ready_mutex_);
        ERaftKvServer::ready_cond_vars_[op_count_]->wait(ul, [] { return true; });
        op_count_ += 1;
      }
    }
  }
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