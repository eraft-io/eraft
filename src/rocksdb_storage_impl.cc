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
 * @file rocksdb_storage_impl.cc
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-04-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "rocksdb_storage_impl.h"

#include <rocksdb/utilities/checkpoint.h>
#include <spdlog/spdlog.h>

#include "consts.h"
#include "eraftkv.pb.h"
#include "eraftkv_server.h"
#include "util.h"

/**
 * @brief Get the Node Address object
 *
 * @param raft
 * @param id
 * @return std::string
 */
std::string RocksDBStorageImpl::GetNodeAddress(RaftServer* raft,
                                               std::string id) {
  return std::string("");
}

/**
 * @brief
 *
 * @param raft
 * @param id
 * @param address
 * @return EStatus
 */
EStatus RocksDBStorageImpl::SaveNodeAddress(RaftServer* raft,
                                            std::string id,
                                            std::string address) {
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @param raft
 * @param snapshot_index
 * @param snapshot_term
 * @return EStatus
 */
EStatus RocksDBStorageImpl::ApplyLog(RaftServer* raft,
                                     int64_t     snapshot_index,
                                     int64_t     snapshot_term) {
  if (raft->commit_idx_ == raft->last_applied_idx_) {
    return EStatus::kOk;
  }
  SPDLOG_INFO("appling entries from {} to {}",
              raft->last_applied_idx_,
              raft->commit_idx_);
  auto etys =
      raft->log_store_->Gets(raft->last_applied_idx_, raft->commit_idx_);
  for (auto ety : etys) {
    switch (ety->e_type()) {
      case eraftkv::EntryType::Normal: {
        eraftkv::KvOpPair* op_pair = new eraftkv::KvOpPair();
        op_pair->ParseFromString(ety->data());
        switch (op_pair->op_type()) {
          case eraftkv::ClientOpType::Put: {
            if (PutKV(op_pair->key(), op_pair->value()) == EStatus::kOk) {
              raft->log_store_->PersisLogMetaState(raft->commit_idx_,
                                                   ety->id());
              raft->last_applied_idx_ = ety->id();
              if (raft->role_ == NodeRaftRoleEnum::Leader) {
                {
                  std::lock_guard<std::mutex> lg(ERaftKvServer::ready_mutex_);
                  ERaftKvServer::is_ok_ = true;
                  if (ERaftKvServer::ready_cond_vars_[op_pair->op_sign()] !=
                      nullptr) {
                    ERaftKvServer::ready_cond_vars_[op_pair->op_sign()]
                        ->notify_one();
                  }
                }
              }
            }
            break;
          }
          case eraftkv::ClientOpType::Del: {
            if (DelKV(op_pair->key()) == EStatus::kOk) {
              raft->log_store_->PersisLogMetaState(raft->commit_idx_,
                                                   ety->id());
              raft->last_applied_idx_ = ety->id();
              if (raft->role_ == NodeRaftRoleEnum::Leader) {
                {
                  std::lock_guard<std::mutex> lg(ERaftKvServer::ready_mutex_);
                  ERaftKvServer::is_ok_ = true;
                  if (ERaftKvServer::ready_cond_vars_[op_pair->op_sign()] !=
                      nullptr) {
                    ERaftKvServer::ready_cond_vars_[op_pair->op_sign()]
                        ->notify_one();
                  }
                }
              }
            }
            break;
          }
          default: {
            raft->log_store_->PersisLogMetaState(raft->commit_idx_, ety->id());
            raft->last_applied_idx_ = ety->id();
            break;
          }
        }
        delete op_pair;
        if (raft->log_store_->LogCount() > raft->snap_threshold_log_count_) {
          raft->SnapshotingStart(ety->id());
        }
        break;
      }
      case eraftkv::EntryType::ConfChange: {
        eraftkv::ClusterConfigChangeReq* conf_change_req =
            new eraftkv::ClusterConfigChangeReq();
        conf_change_req->ParseFromString(ety->data());
        raft->log_store_->PersisLogMetaState(raft->commit_idx_, ety->id());
        raft->last_applied_idx_ = ety->id();
        switch (conf_change_req->change_type()) {
          case eraftkv::ChangeType::ServerJoin: {
            if (conf_change_req->server().id() != raft->id_) {
              RaftNode* new_node =
                  new RaftNode(conf_change_req->server().id(),
                               NodeStateEnum::Running,
                               0,
                               ety->id(),
                               conf_change_req->server().address());
              raft->net_->InsertPeerNodeConnection(
                  conf_change_req->server().id(),
                  conf_change_req->server().address());
              bool node_exist = false;
              for (auto node : raft->nodes_) {
                if (node->id == new_node->id) {
                  node_exist = true;
                  // reinit node
                  if (node->node_state == NodeStateEnum::Down) {
                    SPDLOG_DEBUG("reinit node {} to running state",
                                 conf_change_req->server().address());
                    node->node_state = NodeStateEnum::Running;
                    node->next_log_index = 0;
                    node->match_log_index = ety->id();
                    node->address = conf_change_req->server().address();
                  }
                }
              }
              if (!node_exist) {
                raft->nodes_.push_back(new_node);
              }
            }
            break;
          }
          case eraftkv::ChangeType::ServerLeave: {
            auto to_remove_serverid = conf_change_req->server().id();
            for (auto iter = raft->nodes_.begin(); iter != raft->nodes_.end();
                 iter++) {
              if ((*iter)->id == to_remove_serverid &&
                  conf_change_req->server().id() != raft->id_) {
                (*iter)->node_state = NodeStateEnum::Down;
              }
            }
            break;
          }
          case eraftkv::ChangeType::ShardJoin: {
            std::string key;
            key.append(SG_META_PREFIX);
            key.append(std::to_string(conf_change_req->shard_id()));
            auto        sg = conf_change_req->shard_group();
            std::string val = sg.SerializeAsString();
            raft->store_->PutKV(key, val);
            break;
          }
          case eraftkv::ChangeType::ShardLeave: {
            std::string key;
            key.append(SG_META_PREFIX);
            key.append(std::to_string(conf_change_req->shard_id()));
            raft->store_->DelKV(key);
            break;
          }
          case eraftkv::ChangeType::SlotMove: {
            auto        sg = conf_change_req->shard_group();
            std::string key = SG_META_PREFIX;
            key.append(std::to_string(conf_change_req->shard_id()));
            auto value = raft->store_->GetKV(key);
            if (!value.first.empty()) {
              eraftkv::ShardGroup* old_sg = new eraftkv::ShardGroup();
              old_sg->ParseFromString(value.first);
              // move slot to new sg
              if (sg.id() == old_sg->id()) {
                for (auto new_slot : sg.slots()) {
                  // check if slot already exists
                  bool slot_already_exists = false;
                  for (auto old_slot : old_sg->slots()) {
                    if (old_slot.id() == new_slot.id()) {
                      slot_already_exists = true;
                    }
                  }
                  // add slot to sg
                  if (!slot_already_exists) {
                    auto add_slot = old_sg->add_slots();
                    add_slot->CopyFrom(new_slot);
                  }
                }
                // write back to db
                EStatus st =
                    raft->store_->PutKV(key, old_sg->SerializeAsString());
                assert(st == EStatus::kOk);
              }
            }
            break;
          }
          case eraftkv::ChangeType::ShardsQuery: {
            break;
          }
          default: {
            break;
          }
        }
        std::mutex map_mutex;
        {
          std::lock_guard<std::mutex> lg(map_mutex);
          if (ERaftKvServer::ready_cond_vars_[conf_change_req->op_sign()] !=
              nullptr) {
            ERaftKvServer::ready_cond_vars_[conf_change_req->op_sign()]
                ->notify_one();
          }
        }
        delete conf_change_req;
        break;
      }
      default:
        break;
    }
  }
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @param snap_path
 * @return EStatus
 */
EStatus RocksDBStorageImpl::CreateCheckpoint(std::string snap_path) {
  rocksdb::Checkpoint* checkpoint;
  DirectoryTool::DeleteDir(snap_path);
  auto st = rocksdb::Checkpoint::Create(this->kv_db_, &checkpoint);
  if (!st.ok()) {
    return EStatus::kError;
  }
  auto st_ = checkpoint->CreateCheckpoint(snap_path);
  if (!st_.ok()) {
    return EStatus::kError;
  }
  SPDLOG_INFO("success create db checkpoint in {} ", snap_path);
  return EStatus::kOk;
}


/**
 * @brief
 *
 * @param raft
 * @param term
 * @param vote
 * @return EStatus
 */
EStatus RocksDBStorageImpl::SaveRaftMeta(RaftServer* raft,
                                         int64_t     term,
                                         int64_t     vote) {
  auto status =
      kv_db_->Put(rocksdb::WriteOptions(), "M:TERM", std::to_string(term));
  if (!status.ok()) {
    return EStatus::kError;
  }
  status = kv_db_->Put(rocksdb::WriteOptions(), "M:VOTE", std::to_string(vote));
  if (!status.ok()) {
    return EStatus::kError;
  }
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @param raft
 * @param term
 * @param vote
 * @return EStatus
 */
EStatus RocksDBStorageImpl::ReadRaftMeta(RaftServer* raft,
                                         int64_t*    term,
                                         int64_t*    vote) {
  try {
    std::string term_str;
    auto status = kv_db_->Get(rocksdb::ReadOptions(), "M:TERM", &term_str);
    *term = static_cast<int64_t>(stoi(term_str));
    if (!status.ok()) {
      return EStatus::kError;
    }
    std::string vote_str;
    status = kv_db_->Get(rocksdb::ReadOptions(), "M:VOTE", &vote_str);
    *vote = static_cast<int64_t>(stoi(vote_str));
    if (!status.ok()) {
      return EStatus::kError;
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return EStatus::kError;
  }

  return EStatus::kOk;
}


/**
 * @brief put key and value to kv rocksdb
 *
 * @param key
 * @param val
 * @return EStatus
 */
EStatus RocksDBStorageImpl::PutKV(std::string key, std::string val) {
  SPDLOG_INFO("put key {} value {} to db", key, val);
  auto status = kv_db_->Put(rocksdb::WriteOptions(), "U:" + key, val);
  return status.ok() ? EStatus::kOk : EStatus::kPutKeyToRocksDBErr;
}

/**
 * @brief get value from kv rocksdb
 *
 * @param key
 * @return std::string
 */
std::pair<std::string, bool> RocksDBStorageImpl::GetKV(std::string key) {
  std::string value;
  auto        status = kv_db_->Get(rocksdb::ReadOptions(), "U:" + key, &value);
  return std::make_pair<std::string, bool>(std::move(value),
                                           !status.IsNotFound());
}

/**
 * @brief
 *
 * @param prefix
 * @param offset
 * @param limit
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> RocksDBStorageImpl::PrefixScan(
    std::string prefix,
    int64_t     offset,
    int64_t     limit) {
  auto iter = kv_db_->NewIterator(rocksdb::ReadOptions());
  iter->Seek("U:" + prefix);
  while (iter->Valid() && offset > 0) {
    offset -= 1;
    iter->Next();
  }
  if (!iter->Valid()) {
    return std::map<std::string, std::string>{};
  }
  std::map<std::string, std::string> kvs;
  int64_t                            res_count = 0;
  while (iter->Valid() && limit > res_count) {
    kvs.insert(std::make_pair<std::string, std::string>(
        iter->key().ToString(), iter->value().ToString()));
    iter->Next();
    res_count += 1;
  }
  return kvs;
}

EStatus RocksDBStorageImpl::IngestSST(std::string sst_file_path) {
  rocksdb::IngestExternalFileOptions ifo;
  auto st = kv_db_->IngestExternalFile({sst_file_path}, ifo);
  if (!st.ok()) {
    SPDLOG_ERROR("ingest sst file {} error", sst_file_path);
    return EStatus::kError;
  }
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @param key
 * @return EStatus
 */
EStatus RocksDBStorageImpl::DelKV(std::string key) {
  SPDLOG_DEBUG("del key {}", key);
  auto status = kv_db_->Delete(rocksdb::WriteOptions(), "U:" + key);
  return status.ok() ? EStatus::kOk : EStatus::kDelFromRocksDBErr;
}

/**
 * @brief Construct a new RocksDB Storage Impl object
 *
 * @param db_path
 */
RocksDBStorageImpl::RocksDBStorageImpl(std::string db_path) {
  rocksdb::Options options;
  options.create_if_missing = true;
  rocksdb::Status status = rocksdb::DB::Open(options, db_path, &kv_db_);
  assert(status.ok());
}

/**
 * @brief Destroy the Rocks DB Storage Impl:: RocksDB Storage Impl object
 *
 */
RocksDBStorageImpl::~RocksDBStorageImpl() {
  delete kv_db_;
}
