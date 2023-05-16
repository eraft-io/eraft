#include <rocksdb/db.h>
#include <stdint.h>

#include <iostream>

#include "rocksdb_storage_impl.h"
#include "util.h"


/**
 * @brief
 *
 */
RocksDBLogStorageImpl::RocksDBLogStorageImpl()
    : log_cache_(new LogEntryCache()) {
  // read meta data from log storage
  m_status_.last_log_index = 0;
  s_status_.last_log_index = 0;
}

EStatus RocksDBLogStorageImpl::Reset(int64_t index, int64_t term) {
  return EStatus::kOk;
}

/**
 * @brief init log db when cluster init
 *
 * @param logdb_path
 * @return EStatus
 */
EStatus RocksDBLogStorageImpl::Open(std::string logdb_path,
                                    int64_t     prev_log_term,
                                    int64_t     prev_log_index) {
  // init master log db status
  m_status_.last_log_index = prev_log_index;
  m_status_.prev_log_index = prev_log_index;
  m_status_.prev_log_term = prev_log_term;

  return EStatus::kOk;
}

/**
 * @brief
 *
 */

RocksDBLogStorageImpl::~RocksDBLogStorageImpl() {
  delete log_cache_;
}

/**
 * @brief
 *
 * @param ety
 * @return EStatus
 */
EStatus RocksDBLogStorageImpl::Append(eraftkv::Entry* ety) {
  if (standby_log_db_ != nullptr) {
    // gnerator ety index
    int64_t ety_index = this->s_status_.last_log_index + 1;
    ety->set_id(ety_index);
    std::cout << "append log entry with id: " << ety->id()
              << " index: " << ety_index << std::endl;

    // encode and wirte to rocksdb
    std::string* key = new std::string();
    key->append("RAFTLOG");
    EncodeDecodeTool::PutFixed64(key, static_cast<uint64_t>(ety_index));
    std::string val = ety->SerializeAsString();
    standby_log_db_->Put(rocksdb::WriteOptions(), *key, val);

    // add to cache
    log_cache_->Append(ety);

  } else {
    // gnerator ety index
    int64_t ety_index = this->m_status_.last_log_index + 1;
    ety->set_id(ety_index);
    std::cout << "append log entry with id: " << ety->id()
              << " index: " << ety_index << std::endl;

    // encode and wirte to rocksdb
    std::string* key = new std::string();
    key->append("RAFTLOG");
    EncodeDecodeTool::PutFixed64(key, static_cast<uint64_t>(ety_index));
    std::string val = ety->SerializeAsString();
    master_log_db_->Put(rocksdb::WriteOptions(), *key, val);

    // add to cache
    log_cache_->Append(ety);
  }
  return EStatus::kOk;
}

EStatus Reset(int64_t index, int64_t term) {

  return EStatus::kOk;
}

/**
 * @brief
 *
 * @param first_index
 * @return EStatus
 */
EStatus RocksDBLogStorageImpl::EraseBefore(int64_t first_index) {
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @param from_index
 * @return EStatus
 */
EStatus RocksDBLogStorageImpl::EraseAfter(int64_t from_index) {
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @param index
 * @return eraftkv::Entry*
 */
eraftkv::Entry* RocksDBLogStorageImpl::Get(int64_t index) {
  return nullptr;
}

/**
 * @brief
 *
 * @param start_index
 * @param end_index
 * @return std::vector<eraftkv::Entry*>
 */
std::vector<eraftkv::Entry*> RocksDBLogStorageImpl::Gets(int64_t start_index,
                                                         int64_t end_index) {
  return std::vector<eraftkv::Entry*>{};
}

/**
 * @brief
 *
 * @return int64_t
 */
int64_t RocksDBLogStorageImpl::FirstIndex() {
  return 0;
}

/**
 * @brief
 *
 * @return int64_t
 */
int64_t RocksDBLogStorageImpl::LastIndex() {
  return 0;
}

/**
 * @brief
 *
 * @return int64_t
 */
int64_t RocksDBLogStorageImpl::LogCount() {
  return 0;
}


/**
 * @brief Append add new entries
 *
 * @param ety
 * @return EStatus
 */
EStatus RocksDBSingleLogStorageImpl::Append(eraftkv::Entry* ety) {
  std::string key;
  key.append("E:");
  EncodeDecodeTool::PutFixed64(&key, static_cast<uint64_t>(ety->id()));
  std::string val = ety->SerializeAsString();
  auto        st = log_db_->Put(rocksdb::WriteOptions(), key, val);
  assert(st.ok());
  this->last_idx = ety->id();
  return EStatus::kOk;
}

/**
 * @brief EraseBefore erase all entries before the given index
 *
 * @param first_index
 * @return EStatus
 */
EStatus RocksDBSingleLogStorageImpl::EraseBefore(int64_t first_index) {
  int64_t old_fir_idx = this->first_idx;
  this->first_idx = first_index;
  for (int64_t i = old_fir_idx; i < first_index; i++) {
    std::string key;
    key.append("E:");
    EncodeDecodeTool::PutFixed64(&key, static_cast<uint64_t>(i));
    auto st = log_db_->Delete(rocksdb::WriteOptions(), key);
    assert(st.ok());
  }
  return EStatus::kOk;
}

/**
 * @brief EraseAfter erase all entries after the given index
 *
 * @param from_index
 * @return EStatus
 */
EStatus RocksDBSingleLogStorageImpl::EraseAfter(int64_t from_index) {
  for (int64_t i = from_index; i <= this->last_idx; i++) {
    std::string key;
    key.append("E:");
    EncodeDecodeTool::PutFixed64(&key, static_cast<uint64_t>(i));
    auto st = log_db_->Delete(rocksdb::WriteOptions(), key);
    assert(st.ok());
  }
  this->last_idx = from_index;
  return EStatus::kOk;
}

/**
 * @brief 
 * 
 * @param start 
 * @param end 
 * @return EStatus 
 */
EStatus RocksDBSingleLogStorageImpl::EraseRange(int64_t start, int64_t end)
{
  for (int64_t i = start; i < end; i ++) {
    std::string key;
    key.append("E:");
    EncodeDecodeTool::PutFixed64(&key, static_cast<uint64_t>(i));
    auto st = log_db_->Delete(rocksdb::WriteOptions(), key);
    assert(st.ok());
  }
  return EStatus::kOk;
}

/**
 * @brief Get get the given index entry
 *
 * @param index
 * @return eraftkv::Entry*
 */
eraftkv::Entry* RocksDBSingleLogStorageImpl::Get(int64_t index) {
  eraftkv::Entry* new_ety = new eraftkv::Entry();
  std::string     new_ety_str;
  std::string     key;
  key.append("E:");
  EncodeDecodeTool::PutFixed64(&key, static_cast<uint64_t>(index));
  auto status = log_db_->Get(rocksdb::ReadOptions(), key, &new_ety_str);
  assert(status.ok());
  bool parse_ok = new_ety->ParseFromString(new_ety_str);
  assert(parse_ok);
  return new_ety;
}

/**
 * @brief Gets get the given index range entry
 *
 * @param start_index
 * @param end_index
 * @return std::vector<eraftkv::Entry*>
 */
std::vector<eraftkv::Entry*> RocksDBSingleLogStorageImpl::Gets(
    int64_t start_index,
    int64_t end_index) {
  std::vector<eraftkv::Entry*> entries;
  for (int64_t i = start_index; i <= end_index; i++) {
    auto ety = this->Get(i);
    entries.push_back(ety);
  }
  return entries;
}

eraftkv::Entry* RocksDBSingleLogStorageImpl::GetFirstEty() {
  return this->Get(this->first_idx);
}

eraftkv::Entry* RocksDBSingleLogStorageImpl::GetLastEty() {
  return this->Get(this->last_idx);
}

/**
 * @brief FirstIndex get the first index in the entry
 *
 * @return int64_t
 */
int64_t RocksDBSingleLogStorageImpl::FirstIndex() {
  return this->first_idx;
}

/**
 * @brief LastIndex get the last index in the entry
 *
 * @return int64_t
 */
int64_t RocksDBSingleLogStorageImpl::LastIndex() {
  return this->last_idx;
}

/**
 * @brief LogCount get the number of entries
 *
 * @return int64_t
 */
int64_t RocksDBSingleLogStorageImpl::LogCount() {
  return this->last_idx - this->first_idx + 1;
}

EStatus RocksDBSingleLogStorageImpl::PersisLogMetaState(int64_t commit_idx,
                                                        int64_t applied_idx) {
  auto status = log_db_->Put(
      rocksdb::WriteOptions(), "M:COMMIT_IDX", std::to_string(commit_idx));
  if (status.ok()) {
    return EStatus::kOk;
  } else {
    return EStatus::kError;
  }
  status = log_db_->Put(
      rocksdb::WriteOptions(), "M:APPLIED_IDX", std::to_string(applied_idx));
  if (status.ok()) {
    return EStatus::kOk;
  } else {
    return EStatus::kError;
  }
  status = log_db_->Put(
      rocksdb::WriteOptions(), "M:FIRST_IDX", std::to_string(this->first_idx));
  if (status.ok()) {
    return EStatus::kOk;
  } else {
    return EStatus::kError;
  }
  status = log_db_->Put(
      rocksdb::WriteOptions(), "M:LAST_IDX", std::to_string(this->last_idx));
  if (status.ok()) {
    return EStatus::kOk;
  } else {
    return EStatus::kError;
  }
  status = log_db_->Put(rocksdb::WriteOptions(),
                        "M:SNAP_IDX",
                        std::to_string(this->snapshot_idx));
  if (status.ok()) {
    return EStatus::kOk;
  } else {
    return EStatus::kError;
  }
}

EStatus RocksDBSingleLogStorageImpl::ReadMetaState(int64_t* commit_idx,
                                                   int64_t* applied_idx) {
  try
  {
    std::string commit_idx_str;
    auto        status =
        log_db_->Get(rocksdb::ReadOptions(), "M:COMMIT_IDX", &commit_idx_str);
    *commit_idx = static_cast<int64_t>(stoi(commit_idx_str));
    if (status.ok()) {
      return EStatus::kOk;
    } else {
      return EStatus::kError;
    }
    std::string applied_idx_str;
    status =
        log_db_->Get(rocksdb::ReadOptions(), "M:APPLIED_IDX", &applied_idx_str);
    *applied_idx = static_cast<int64_t>(stoi(applied_idx_str));
    if (status.ok()) {
      return EStatus::kOk;
    } else {
      return EStatus::kError;
    }
    std::string first_idx_str;
    status = log_db_->Get(rocksdb::ReadOptions(), "M:FIRST_IDX", &first_idx_str);
    if (status.ok()) {
      return EStatus::kOk;
    } else {
      return EStatus::kError;
    }
    this->first_idx = static_cast<int64_t>(stoi(first_idx_str));
    std::string last_idx_str;
    status = log_db_->Get(rocksdb::ReadOptions(), "M:LAST_IDX", &last_idx_str);
    if (status.ok()) {
      return EStatus::kOk;
    } else {
      return EStatus::kError;
    }
    this->last_idx = static_cast<int64_t>(stoi(last_idx_str));
    std::string snap_idx_str;
    status = log_db_->Get(rocksdb::ReadOptions(), "M:SNAP_IDX", &snap_idx_str);
    if (status.ok()) {
      return EStatus::kOk;
    } else {
      return EStatus::kError;
    }
    this->snapshot_idx = static_cast<int64_t>(stoi(snap_idx_str));
    return EStatus::kOk;
  }
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';
    return EStatus::kError;
  }
  return EStatus::kOk;
}

RocksDBSingleLogStorageImpl::RocksDBSingleLogStorageImpl(std::string db_path)
    : first_idx(0), last_idx(0), snapshot_idx(0) {
  rocksdb::Options options;
  options.create_if_missing = true;
  rocksdb::Status status = rocksdb::DB::Open(options, db_path, &log_db_);
  TraceLog("DEBUG: ", "init log db success with path ", db_path);
  // if not log meta, init log
  int64_t commit_idx, applied_idx;
  auto    est = ReadMetaState(&commit_idx, &applied_idx);
  if (est == EStatus::kError) {
    eraftkv::Entry* ety = new eraftkv::Entry();
    // write init log with index 0 to rocksdb
    std::string* key = new std::string();
    key->append("E:");
    EncodeDecodeTool::PutFixed64(key, static_cast<uint64_t>(0));
    std::string val = ety->SerializeAsString();
    auto        status = log_db_->Put(rocksdb::WriteOptions(), *key, val);
    assert(status.ok());
  }
}

RocksDBSingleLogStorageImpl::~RocksDBSingleLogStorageImpl() {
  delete log_db_;
}
