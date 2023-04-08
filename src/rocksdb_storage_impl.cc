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

/**
 * @brief Get the Node Address object
 *
 * @param raft
 * @param id
 * @return std::string
 */
std::string RocksDBStorageImpl::GetNodeAddress(RaftServer* raft,
                                               std::string id) {}

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
                                            std::string address) {}

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
                                     int64_t     snapshot_term) {}

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
                                             eraftkv::SnapshotBlock* block) {}

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
                                               eraftkv::SnapshotBlock* block) {}

/**
 * @brief
 *
 * @param raft
 * @return EStatus
 */
EStatus RocksDBStorageImpl::ClearSnapshot(RaftServer* raft) {}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RocksDBStorageImpl::CreateDBSnapshot() {}

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
                                         int64_t     vote) {}

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
                                         int64_t*    vote) {}


/**
 * @brief put key and value to kv rocksdb
 *
 * @param key
 * @param val
 * @return EStatus
 */
EStatus RocksDBStorageImpl::PutKV(std::string key, std::string val) {
  auto status = kv_db_->Put(rocksdb::WriteOptions(), key, val);
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
  auto        status = kv_db_->Get(rocksdb::ReadOptions(), key, &value);
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
}

/**
 * @brief Destroy the Rocks D B Storage Impl:: RocksDB Storage Impl object
 *
 */
RocksDBStorageImpl::~RocksDBStorageImpl() {
  delete kv_db_;
}


/**
 * @brief
 *
 */
void RocksDBLogStorageImpl::Init() {}

/**
 * @brief
 *
 */
void RocksDBLogStorageImpl::Free() {}

/**
 * @brief
 *
 * @param ety
 * @return EStatus
 */
EStatus RocksDBLogStorageImpl::Append(eraftkv::Entry* ety) {}

/**
 * @brief
 *
 * @param first_index
 * @return EStatus
 */
EStatus RocksDBLogStorageImpl::EraseBefore(int64_t first_index) {}

/**
 * @brief
 *
 * @param from_index
 * @return EStatus
 */
EStatus RocksDBLogStorageImpl::EraseAfter(int64_t from_index) {}

/**
 * @brief
 *
 * @param index
 * @return eraftkv::Entry*
 */
eraftkv::Entry* RocksDBLogStorageImpl::Get(int64_t index) {}

/**
 * @brief
 *
 * @param start_index
 * @param end_index
 * @return std::vector<eraftkv::Entry*>
 */
std::vector<eraftkv::Entry*> RocksDBLogStorageImpl::Gets(int64_t start_index,
                                                         int64_t end_index) {}

/**
 * @brief
 *
 * @return int64_t
 */
int64_t RocksDBLogStorageImpl::FirstIndex() {}

/**
 * @brief
 *
 * @return int64_t
 */
int64_t RocksDBLogStorageImpl::LastIndex() {}

/**
 * @brief
 *
 * @return int64_t
 */
int64_t RocksDBLogStorageImpl::LogCount() {}
