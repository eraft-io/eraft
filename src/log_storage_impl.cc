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
    : log_cache_(new LogEntryCache())
    , m_logdb_lastindex_(0)
    , s_logdb_lastindex_(0) {
      
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
    int64_t ety_index = s_logdb_lastindex_ + 1;
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
    int64_t ety_index = m_logdb_lastindex_ + 1;
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
}

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
