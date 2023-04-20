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
