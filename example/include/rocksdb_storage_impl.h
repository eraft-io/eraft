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
 * @file rocksdb_storage_impl.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <rocksdb/db.h>

#include "eraft/raft_server.h"
#include "log_entry_cache.h"

/**
 * @brief
 *
 */
class RocksDBStorageImpl : public Storage {

 public:
  /**
   * @brief
   *
   * @param raft
   * @param term
   * @param vote
   * @return EStatus
   */
  EStatus SaveRaftMeta(RaftServer* raft, int64_t term, int64_t vote);

  /**
   * @brief
   *
   * @param raft
   * @param term
   * @param vote
   * @return EStatus
   */
  EStatus ReadRaftMeta(RaftServer* raft, int64_t* term, int64_t* vote);

  /**
   * @brief
   *
   * @param key
   * @param val
   * @return EStatus
   */
  EStatus PutKV(std::string key, std::string val);

  /**
   * @brief
   *
   * @param key
   * @return std::string
   */
  std::pair<std::string, bool> GetKV(std::string key);

  /**
   * @brief
   *
   * @param prefix
   * @param offset
   * @param limit
   * @return std::map<std::string, std::string>
   */
  std::map<std::string, std::string> PrefixScan(std::string prefix,
                                                int64_t     offset,
                                                int64_t     limit);

  /**
   * @brief
   *
   * @param sst_file_path
   * @return EStatus
   */
  EStatus IngestSST(std::string sst_file_path);

  /**
   * @brief
   *
   * @param snap_base_path
   * @param sst_file_path
   * @return EStatus
   */
  EStatus ProductSST(std::string snap_base_path, std::string sst_file_path);

  /**
   * @brief
   *
   * @param key
   * @return EStatus
   */
  EStatus DelKV(std::string key);

  /**
   * @brief Construct a new RocksDB Storage Impl object
   *
   * @param db_path
   */
  RocksDBStorageImpl(std::string db_path);

  /**
   * @brief Destroy the Rocks DB Storage Impl object
   *
   */
  ~RocksDBStorageImpl();

  /**
   * @brief Create a Checkpoint object
   *
   * @param snap_path
   * @return EStatus
   */
  EStatus CreateCheckpoint(std::string snap_path);

 private:
  /**
   * @brief
   *
   */
  std::string db_path_;

  /**
   * @brief
   *
   */
  rocksdb::DB* kv_db_;
};


class RocksDBSingleLogStorageImpl : public LogStore {

 public:
  RocksDBSingleLogStorageImpl(std::string db_path);

  ~RocksDBSingleLogStorageImpl();

  /**
   * @brief Append add new entries
   *
   * @param ety
   * @return EStatus
   */
  EStatus Append(eraftkv::Entry* ety);

  /**
   * @brief
   *
   * @param new_idx
   */
  void ResetFirstIndex(int64_t new_idx);

  /**
   * @brief
   *
   * @param term
   * @param index
   */
  void ResetFirstLogEntry(int64_t term, int64_t index);


  /**
   * @brief EraseBefore erase all entries before the given index
   *
   * @param first_index
   * @return EStatus
   */
  EStatus EraseBefore(int64_t first_index);

  /**
   * @brief EraseAfter erase all entries after the given index
   *
   * @param from_index
   * @return EStatus
   */
  EStatus EraseAfter(int64_t from_index);

  /**
   * @brief Get get the given index entry
   *
   * @param index
   * @return eraftkv::Entry*
   */
  eraftkv::Entry* Get(int64_t index);

  /**
   * @brief Get the First Ety object
   *
   * @return eraftkv::Entry*
   */
  eraftkv::Entry* GetFirstEty();

  /**
   * @brief Get the Last Ety object
   *
   * @return eraftkv::Entry*
   */
  eraftkv::Entry* GetLastEty();

  /**
   * @brief
   *
   * @param start
   * @param end
   * @return EStatus
   */
  EStatus EraseRange(int64_t start, int64_t end);

  /**
   * @brief Gets get the given index range entry
   *
   * @param start_index
   * @param end_index
   * @return std::vector<eraftkv::Entry*>
   */
  std::vector<eraftkv::Entry*> Gets(int64_t start_index, int64_t end_index);

  /**
   * @brief FirstIndex get the first index in the entry
   *
   * @return int64_t
   */
  int64_t FirstIndex();

  /**
   * @brief LastIndex get the last index in the entry
   *
   * @return int64_t
   */
  int64_t LastIndex();

  EStatus Reinit();

  /**
   * @brief LogCount get the number of entries
   *
   * @return int64_t
   */
  int64_t LogCount();

  EStatus PersisLogMetaState(int64_t commit_idx, int64_t applied_idx);

  EStatus ReadMetaState(int64_t* commit_idx, int64_t* applied_idx);

  int64_t first_idx;

  int64_t last_idx;

  int64_t snapshot_idx;

 private:
  int64_t commit_idx_;

  int64_t applied_idx_;


  rocksdb::DB* log_db_;
};
