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
#include <spdlog/spdlog.h>

#include <chrono>
#include <iostream>

#include "consts.h"
#include "file_reader_into_stream.h"
#include "sequential_file_reader.h"
#include "sequential_file_writer.h"

RaftServer* ERaftKvServer::raft_context_ = nullptr;

std::map<int, std::condition_variable*> ERaftKvServer::ready_cond_vars_;

std::mutex ERaftKvServer::ready_mutex_;

bool ERaftKvServer::is_ok_ = false;

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
  if (raft_context_->HandleSnapshotReq(nullptr, req, resp) == EStatus::kOk) {
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
grpc::Status ERaftKvServer::ProcessRWOperation(
    ServerContext*                     context,
    const eraftkv::ClientOperationReq* req,
    eraftkv::ClientOperationResp*      resp) {
  int64_t log_index;
  int64_t log_term;
  bool    success;
  SPDLOG_INFO(
      "recv rw op with ts {} {}", req->op_timestamp(), req->DebugString());
  // no leader reject
  if (!raft_context_->IsLeader()) {
    resp->set_error_code(eraftkv::ErrorCode::REQUEST_NOT_LEADER_NODE);
    resp->set_leader_addr(raft_context_->GetLeaderId());
    return grpc::Status::OK;
  }
  // snapshot reject
  if (raft_context_->IsSnapshoting()) {
    SPDLOG_WARN("node is snapshoting, reject request");
    resp->set_error_code(eraftkv::ErrorCode::NODE_IS_SNAPSHOTING);
    return grpc::Status::OK;
  }
  for (auto kv_op : req->kvs()) {
    int rand_seq = static_cast<int>(RandomNumber::Between(1, 100000));
    SPDLOG_INFO("recv rw op type {} op count {}", kv_op.op_type(), rand_seq);
    switch (kv_op.op_type()) {
      case eraftkv::ClientOpType::Get: {
        auto val = raft_context_->store_->GetKV(kv_op.key());
        SPDLOG_INFO(" get key {}  with value {}", kv_op.key(), val.first);
        auto res = resp->add_ops();
        res->set_key(kv_op.key());
        res->set_value(val.first);
        res->set_success(val.second);
        res->set_op_type(eraftkv::ClientOpType::Get);
        res->set_op_sign(kv_op.op_sign());
        break;
      }
      case eraftkv::ClientOpType::Put:
      case eraftkv::ClientOpType::Del: {
        std::mutex map_mutex_;
        {
          std::condition_variable*    new_var = new std::condition_variable();
          std::lock_guard<std::mutex> lg(map_mutex_);
          ERaftKvServer::ready_cond_vars_[rand_seq] = new_var;
          kv_op.set_op_sign(rand_seq);
        }
        raft_context_->Propose(
            kv_op.SerializeAsString(), &log_index, &log_term, &success);
        {
          auto endTime =
              std::chrono::system_clock::now() + std::chrono::seconds(5);
          std::unique_lock<std::mutex> ul(ERaftKvServer::ready_mutex_);
          // ERaftKvServer::ready_cond_vars_[rand_seq]->wait(
          //     ul, []() { return ERaftKvServer::is_ok_; });
          ERaftKvServer::ready_cond_vars_[rand_seq]->wait_until(
              ul, endTime, []() { return ERaftKvServer::is_ok_; });
          // ERaftKvServer::ready_cond_vars_.erase(rand_seq);
          ERaftKvServer::is_ok_ = false;
          auto res = resp->add_ops();
          res->set_key(kv_op.key());
          res->set_value(kv_op.value());
          res->set_success(true);
          res->set_op_type(kv_op.op_type());
          res->set_op_sign(rand_seq);
        }
        break;
      }
      default:
        break;
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
  SPDLOG_INFO("recv config change req with change_type {} ",
              req->change_type());
  // return cluster topology, Currently, only single raft group are supported
  auto conf_change_req = const_cast<eraftkv::ClusterConfigChangeReq*>(req);
  switch (conf_change_req->change_type()) {
    case eraftkv::ChangeType::ShardsQuery: {
      resp->set_success(true);
      auto kvs = raft_context_->store_->PrefixScan(SG_META_PREFIX, 0, 256);
      for (auto kv : kvs) {
        eraftkv::ShardGroup* sg = new eraftkv::ShardGroup();
        sg->ParseFromString(kv.second);
        auto new_sg = resp->add_shard_group();
        new_sg->CopyFrom(*sg);
        delete sg;
      }
      return grpc::Status::OK;
    }
    case eraftkv::ChangeType::MembersQuery: {
      resp->set_success(true);
      auto new_sg = resp->add_shard_group();
      new_sg->set_id(0);
      for (auto node : raft_context_->GetNodes()) {
        auto g_server = new_sg->add_servers();
        g_server->set_id(node->id);
        g_server->set_address(node->address);
        node->node_state == NodeStateEnum::Running
            ? g_server->set_server_status(eraftkv::ServerStatus::Up)
            : g_server->set_server_status(eraftkv::ServerStatus::Down);
      }
      new_sg->set_leader_id(raft_context_->GetLeaderId());
      return grpc::Status::OK;
    }
    default: {
      // no leader reject
      if (!raft_context_->IsLeader()) {
        resp->set_error_code(eraftkv::ErrorCode::REQUEST_NOT_LEADER_NODE);
        resp->set_leader_addr(raft_context_->GetLeaderId());
        return grpc::Status::OK;
      }

      if (raft_context_->IsSnapshoting()) {
        resp->set_error_code(eraftkv::ErrorCode::NODE_IS_SNAPSHOTING);
        resp->set_leader_addr(raft_context_->GetLeaderId());
        return grpc::Status::OK;
      }

      int        rand_seq = static_cast<int>(RandomNumber::Between(1, 10000));
      std::mutex map_mutex_;
      {
        std::lock_guard<std::mutex> lg(map_mutex_);
        std::condition_variable*    new_var = new std::condition_variable();
        conf_change_req->set_op_sign(rand_seq);
      }
      bool success;
      raft_context_->ProposeConfChange(conf_change_req->SerializeAsString(),
                                       &log_index,
                                       &log_term,
                                       &success);

      {
        std::unique_lock<std::mutex> ul(ERaftKvServer::ready_mutex_);
        ERaftKvServer::ready_cond_vars_[rand_seq]->wait(ul,
                                                        [] { return true; });
        ERaftKvServer::ready_cond_vars_.erase(rand_seq);
      }
      break;
    }
  }
  return grpc::Status::OK;
}

grpc::Status ERaftKvServer::PutSSTFile(
    ServerContext*                               context,
    grpc::ServerReader<eraftkv::SSTFileContent>* reader,
    eraftkv::SSTFileId*                          fileId) {
  eraftkv::SSTFileContent sst_file;
  SequentialFileWriter    writer;
  DirectoryTool::MkDir("/eraft/data/sst_recv/");
  uint64_t sec = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count();
  SPDLOG_INFO(
      "recv sst filename {} id {} sec {}", sst_file.name(), sst_file.id(), sec);
  writer.OpenIfNecessary("/eraft/data/sst_recv/" + std::to_string(sec) +
                         ".sst");
  while (reader->Read(&sst_file)) {
    try {
      auto* const data = sst_file.mutable_content();
      writer.Write(*data);
    } catch (const std::exception& e) {
      std::cerr << e.what() << '\n';
    }
  }
  return grpc::Status::OK;
}


EStatus ERaftKvServer::TakeSnapshot(int64_t log_idx) {
  return raft_context_->SnapshotingStart(log_idx);
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
