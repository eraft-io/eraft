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
 * @file eraftkv_server.cc
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-04-01
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "eraftkv_server.h"

#include <grpcpp/grpcpp.h>

#include "util.h"

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
  if (raft_context_->HandleRequestVoteReq(nullptr, req, resp) == EStatus::kOk) {
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
  if (raft_context_->HandleAppendEntriesReq(nullptr, req, resp) ==
      EStatus::kOk) {
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
  for (auto kv_op : req->kvs()) {
    if (kv_op.op_type() == eraftkv::ClientOpType::Put) {
      std::mutex map_mutex_;
      {
        op_count_ += 1;
        std::condition_variable*    new_var = new std::condition_variable();
        std::lock_guard<std::mutex> lg(map_mutex_);
        ERaftKvServer::ready_cond_vars_[op_count_] = new_var;
        kv_op.set_op_count(op_count_);
      }
      raft_context_->Propose(
          kv_op.SerializeAsString(), &log_index, &log_term, &success);
      {
        std::unique_lock<std::mutex> ul(ERaftKvServer::ready_mutex_);
        ERaftKvServer::ready_cond_vars_[op_count_]->wait(ul,
                                                         [] { return true; });
        ERaftKvServer::ready_cond_vars_.erase(op_count_);
      }
      auto res = resp->add_ops();
      res->set_key(kv_op.key());
      res->set_value(kv_op.value());
      res->set_success(true);
      res->set_op_type(eraftkv::ClientOpType::Put);
      res->set_op_count(op_count_);
    }
    if (kv_op.op_type() == eraftkv::ClientOpType::Get) {
      auto val = raft_context_->store_->GetKV(kv_op.key());
      TraceLog("DEBUG: ", " get key ", kv_op.key(), " with value ", val);
      auto res = resp->add_ops();
      res->set_key(kv_op.key());
      res->set_value(val);
      res->set_success(true);
      res->set_op_type(eraftkv::ClientOpType::Get);
      res->set_op_count(op_count_);
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
  int64_t log_index;
  int64_t log_term;
  bool success;
  TraceLog("DEBUG: ", " recv config change req with change_type ", req->change_type());

  auto conf_change_req = const_cast<eraftkv::ClusterConfigChangeReq*>(req);

  std::mutex map_mutex_;
  {
    op_count_ += 1;
    std::condition_variable* new_var = new std::condition_variable();
    std::lock_guard<std::mutex> lg(map_mutex_);
    conf_change_req->set_op_count(op_count_);
  }

  raft_context_->ProposeConfChange(conf_change_req->SerializeAsString(), &log_index, &log_term, &success);

  {
    std::unique_lock<std::mutex> ul(ERaftKvServer::ready_mutex_);
    ERaftKvServer::ready_cond_vars_[op_count_]->wait(ul,
                                                      [] { return true; });
    ERaftKvServer::ready_cond_vars_.erase(op_count_);
  }

  return success ? grpc::Status::OK : grpc::Status::CANCELLED;
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus ERaftKvServer::BuildAndRunRpcServer() {
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