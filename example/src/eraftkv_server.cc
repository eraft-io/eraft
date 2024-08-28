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

#include "client.h"
#include "file_reader_into_stream.h"
#include "httplib.h"
#include "sequential_file_reader.h"
#include "sequential_file_writer.h"

RaftServer* ERaftKvServer::raft_context_ = nullptr;

std::map<int, std::condition_variable*>*
    ERaftKvServer::response_ready_singals_ = nullptr;

std::mutex* ERaftKvServer::response_ready_mutex_ = nullptr;

bool* ERaftKvServer::is_ok_to_response_ = nullptr;

std::atomic<std::string*> ERaftKvServer::stat_json_str_ = nullptr;

std::atomic<std::string*> ERaftKvServer::cluster_stats_json_str_ = nullptr;

int ERaftKvServer::svr_role_ = -1;

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
          std::condition_variable* new_signal = new std::condition_variable();
          std::lock_guard<std::mutex> lg(map_mutex_);
          (*ERaftKvServer::response_ready_singals_)[rand_seq] = new_signal;
          kv_op.set_op_sign(rand_seq);
        }
        raft_context_->Propose(
            kv_op.SerializeAsString(), &log_index, &log_term, &success);
        {
          auto end_time =
              std::chrono::system_clock::now() +
              std::chrono::seconds(DEAFULT_READ_WRITE_TIMEOUT_SECONDS);
          std::unique_lock<std::mutex> ul(*response_ready_mutex_);
          (*ERaftKvServer::response_ready_singals_)[rand_seq]->wait_until(
              ul, end_time, []() { return *is_ok_to_response_; });
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

      int  rand_seq = static_cast<int>(RandomNumber::Between(1, 10000));
      bool success;
      raft_context_->ProposeConfChange(conf_change_req->SerializeAsString(),
                                       &log_index,
                                       &log_term,
                                       &success);
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
  DirectoryTool::MkDir(raft_context_->snap_db_path_ + "/sst_recv/");
  uint64_t sec = std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::system_clock::now().time_since_epoch())
                     .count();
  SPDLOG_INFO(
      "recv sst filename {} id {} sec {}", sst_file.name(), sst_file.id(), sec);
  writer.OpenIfNecessary(raft_context_->snap_db_path_ + "/sst_recv/" +
                         std::to_string(sec) + ".sst");
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

void ERaftKvServer::UpdateMetaStats() {
  try {
    if (raft_context_->GetLeaderId() == -1) {
      return;
    }
    auto kvs = raft_context_->store_->PrefixScan(SG_META_PREFIX, 0, 256);
    for (auto kv : kvs) {
      eraftkv::ShardGroup* sg = new eraftkv::ShardGroup();
      sg->ParseFromString(kv.second);
      auto sg_leader_id = sg->leader_id();
      *cluster_stats_json_str_ = "[";
      auto shard_svr_addrs =
          StringUtil::Split(DEFAULT_SHARD_MONITOR_ADDRS, ',');
      for (auto shard_svr : shard_svr_addrs) {
        httplib::Client cli(shard_svr);
        cli.set_connection_timeout(0, 200);
        cli.set_read_timeout(0, 200);
        cli.set_write_timeout(0, 200);
        auto res = cli.Get("/collect_stats");
      }
      for (auto server : sg->servers()) {
        *cluster_stats_json_str_ += ("{\"id\":" + std::to_string(server.id()));
        *cluster_stats_json_str_ += (",\"addr\":\"" + server.address() + "\"");
        if (sg_leader_id == server.id()) {
          *cluster_stats_json_str_ += ",\"state\":\"L\"";
        } else {
          *cluster_stats_json_str_ += ",\"state\":\"F\"";
        }
        *cluster_stats_json_str_ += "},";
      }
      (*cluster_stats_json_str_).pop_back();
      *cluster_stats_json_str_ += "]";
      delete sg;
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
  }
}

void ERaftKvServer::ReportStats() {
  try {
    if (raft_context_->GetLeaderId() == -1) {
      return;
    }
    auto        nodes = raft_context_->GetNodes();
    std::string group_server_addresses = "";
    for (auto node : nodes) {
      group_server_addresses += node->address + ",";
    }
    group_server_addresses.pop_back();
    Client metaserver_cli = Client();
    metaserver_cli.AddServerGroupToMeta(
        1, raft_context_->GetLeaderId(), group_server_addresses);
    *stat_json_str_ = "{";
    *stat_json_str_ +=
        "\"commit_index\":" + std::to_string(raft_context_->GetCommitIndex()) +
        ",";
    *stat_json_str_ += "\"applied_index\":" +
                       std::to_string(raft_context_->GetAppliedIndex()) + ",";
    *stat_json_str_ += "\"prefix_logs\":[";
    for (auto log : raft_context_->GetPrefixLogs()) {
      *stat_json_str_ += "{\"index\":" + std::to_string(log->id()) + ",";
      *stat_json_str_ += "\"term\":" + std::to_string(log->term()) + ",";
      switch (log->e_type()) {
        case eraftkv::EntryType::Normal: {
          eraftkv::KvOpPair* op_pair = new eraftkv::KvOpPair();
          op_pair->ParseFromString(log->data());
          *stat_json_str_ +=
              "\"val\":\"" + op_pair->value().substr(0, 6) + "\"},";
          break;
        }
        default:
          break;
      }
    }
    (*stat_json_str_).pop_back();
    *stat_json_str_ += "],\"suffix_logs\":[";
    for (auto log : raft_context_->GetSuffixLogs()) {
      *stat_json_str_ += "{\"index\":" + std::to_string(log->id()) + ",";
      *stat_json_str_ += "\"term\":" + std::to_string(log->term()) + ",";
      switch (log->e_type()) {
        case eraftkv::EntryType::Normal: {
          eraftkv::KvOpPair* op_pair = new eraftkv::KvOpPair();
          op_pair->ParseFromString(log->data());
          *stat_json_str_ +=
              "\"val\":\"" + op_pair->value().substr(0, 6) + "\"},";
          break;
        }
        default:
          break;
      }
    }
    (*stat_json_str_).pop_back();
    *stat_json_str_ += "]}";
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
  }
}