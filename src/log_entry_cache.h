
#ifndef LOG_ENTRY_CACHE_H_
#define LOG_ENTRY_CACHE_H_

#include <stdint.h>

#include <deque>

#include "eraftkv.pb.h"


class LogEntryCache {

 public:
  /**
   * @brief Construct a new Log Entry Cache object
   *
   */
  LogEntryCache();

  /**
   * @brief Destroy the Log Entry Cache object
   *
   */
  ~LogEntryCache();

  /**
   * @brief append a log entry to cache
   *   !!! IMPORTTANT !!!
   *   we assume the first log entry id must > 0
   * @param e
   */
  void Append(eraftkv::Entry* e);

  /**
   * @brief get log entry from log entry cache with index = idx
   *
   * @param idx
   * @return eraftkv::Entry*
   */
  eraftkv::Entry* Get(int64_t idx);

  /**
   * @brief erase log entry with id < idx, idx is the first log of the remaining
   * log structure
   *
   * @param idx
   * @return int64_t
   */
  int64_t EraseHead(int64_t idx);

  /**
   * @brief erase all the log entries wiht index >= idx (including idx)
   *
   * @param idx
   * @return int64_t
   */
  int64_t EraseTail(int64_t idx);

  /**
   * @brief
   *
   * @param max_mem
   * @return int64_t
   */
  int64_t Compact(int64_t max_mem);

  /**
   * @brief return the memsize of all log entries
   *
   * @return uint64_t
   */
  uint64_t MemSize();

  /**
   * @brief return log entry count in log cache
   *
   * @return uint64_t
   */
  uint64_t EntryCount();

  /**
   * @brief
   *
   * @brief return the first log index in log cache
   */
  int64_t FirstIndex();

 private:
  std::deque<eraftkv::Entry*> entries_;

  int64_t first_ent_index_;

  uint64_t mem_size_;

  uint64_t ent_count_;
};

#endif
