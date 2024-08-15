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

#include "eraft/util.h"
#include "eraftkv_server.h"
#include "protocol/eraftkv.pb.h"

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

EStatus RocksDBStorageImpl::ProductSST(std::string snap_base_path,
                                       std::string sst_file_path) {
  RocksDBStorageImpl* snapshot_db =
      new RocksDBStorageImpl(snap_base_path + "/check");
  auto kvs = snapshot_db->PrefixScan("", 0, SNAPSHOTING_KEY_SCAN_PRE_COOUNT);
  DirectoryTool::MkDir(snap_base_path + sst_file_path);
  uint64_t count = 1;
  while (kvs.size() != 0) {
    SPDLOG_INFO("scan find {} keys", kvs.size());
    rocksdb::Options       options;
    rocksdb::SstFileWriter sst_file_writer(rocksdb::EnvOptions(), options);
    sst_file_writer.Open(sst_file_path + std::to_string(count) + ".sst");
    for (auto kv : kvs) {
      SPDLOG_INFO("key {} -> val {}", kv.first, kv.second);
      sst_file_writer.Put(kv.first, kv.second);
    }
    sst_file_writer.Finish();
    kvs = snapshot_db->PrefixScan("",
                                  count * SNAPSHOTING_KEY_SCAN_PRE_COOUNT,
                                  SNAPSHOTING_KEY_SCAN_PRE_COOUNT);
    count += 1;
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
