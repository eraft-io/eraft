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
  auto etys =
      raft->log_store_->Gets(raft->last_applied_idx_, raft->commit_idx_);
  for (auto ety : etys) {
    eraftkv::KvOpPair* op_pair = new eraftkv::KvOpPair();
    op_pair->ParseFromString(ety->data());
    switch (op_pair->op_type()) {
      case eraftkv::ClientOpType::Put: {
        if (PutKV(op_pair->key(), op_pair->value()) == EStatus::kOk) {
          raft->log_store_->PersisLogMetaState(raft->commit_idx_, ety->id());
          raft->last_applied_idx_ = ety->id();
          if (raft->role_ == NodeRaftRoleEnum::Leader) {
            std::mutex map_mutex;
            {
              std::lock_guard<std::mutex> lg(map_mutex);
              ERaftKvServer::ready_cond_vars_[op_pair->op_count()]
                  ->notify_one();
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
  }
  return EStatus::kOk;
}

/**
 * @brief Get the Snapshot Block object
 *
 * @param raft
 * @param node
 * @param offset
 * @param block
 * @return EStatus
 */
EStatus RocksDBStorageImpl::GetSnapshotBlock(RaftServer*             raft,
                                             RaftNode*               node,
                                             int64_t                 offset,
                                             eraftkv::SnapshotBlock* block) {
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @param raft
 * @param snapshot_index
 * @param offset
 * @param block
 * @return EStatus
 */
EStatus RocksDBStorageImpl::StoreSnapshotBlock(RaftServer* raft,
                                               int64_t     snapshot_index,
                                               int64_t     offset,
                                               eraftkv::SnapshotBlock* block) {
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @param raft
 * @return EStatus
 */
EStatus RocksDBStorageImpl::ClearSnapshot(RaftServer* raft) {
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RocksDBStorageImpl::CreateDBSnapshot() {
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
  TraceLog("DEBUG: ", " put key ", key, " value ", val, " to kv db");
  auto status = kv_db_->Put(rocksdb::WriteOptions(), "U:" + key, val);
  return status.ok() ? EStatus::kOk : EStatus::kPutKeyToRocksDBErr;
}

/**
 * @brief get value from kv rocksdb
 *
 * @param key
 * @return std::string
 */
std::string RocksDBStorageImpl::GetKV(std::string key) {
  std::string value;
  auto        status = kv_db_->Get(rocksdb::ReadOptions(), "U:" + key, &value);
  return status.IsNotFound() ? "" : value;
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
  TraceLog("DEBUG: ", "init rocksdb with path ", db_path);
}

/**
 * @brief Destroy the Rocks D B Storage Impl:: RocksDB Storage Impl object
 *
 */
RocksDBStorageImpl::~RocksDBStorageImpl() {
  delete kv_db_;
}
