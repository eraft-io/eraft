/**
 *
 */

#include "log_entry_cache.h"

#include <iostream>

/**
 * @brief Construct a new Log Entry Cache:: Log Entry Cache object
 *
 */
LogEntryCache::LogEntryCache() : ent_count_(0), first_ent_index_(0) {}

/**
 * @brief Destroy the Log Entry Cache:: Log Entry Cache object
 *
 */
LogEntryCache::~LogEntryCache() {}

/**
 * @brief append a log entry to cache
 *   !!! IMPORTTANT !!!
 *   we assume the first log entry id must > 0
 * @param e
 */
void LogEntryCache::Append(eraftkv::Entry* e) {
  // if first ent index not set, set it to e.id
  if (!first_ent_index_) {
    first_ent_index_ = e->id();
  }

  entries_.push_back(e);
  /**
   * @brief
   *
   * Recursively calls ByteSize() on all contained messages.
   * Returns: The number of bytes required to serialize this message. Return
   * type:.
   *
   */
  ent_count_ += 1;
  mem_size_ += e->ByteSizeLong();
}

/**
 * @brief get log entry from log entry cache with index = idx
 *
 * @param idx
 * @return eraftkv::Entry*
 */
eraftkv::Entry* LogEntryCache::Get(int64_t idx) {
  // log cache has be compact, log with idx has already release
  if (idx < first_ent_index_) {
    return nullptr;
  }

  int64_t entry_offset = idx - first_ent_index_;
  if (entry_offset >= ent_count_) {
    return nullptr;
  }

  return entries_.at(entry_offset);
}

/**
 * @brief erase log entry with id < idx, idx is the first log of the remaining
 * log structure
 *
 * @param idx
 * @return int64_t
 */
int64_t LogEntryCache::EraseHead(int64_t idx) {
  if (idx < first_ent_index_) {
    return -1;
  }

  auto begin_ent_it = entries_.begin();
  auto elem_count = idx - first_ent_index_;
  entries_.erase(begin_ent_it, begin_ent_it + elem_count);
  first_ent_index_ += elem_count;
  ent_count_ -= elem_count;
  return elem_count;
}

/**
 * @brief erase all the log entries wiht index >= idx (including idx)
 *
 * @param idx
 * @return int64_t
 */
int64_t LogEntryCache::EraseTail(int64_t idx) {
  if (idx >= first_ent_index_ + entries_.size()) {
    return -1;
  }
  if (idx < first_ent_index_) {
    idx = first_ent_index_;
  }
  auto end_ent_it = entries_.end();
  auto elem_count = entries_.size() - (idx - first_ent_index_);
  entries_.erase(end_ent_it - elem_count, end_ent_it);
  ent_count_ -= elem_count;
  return elem_count;
}

/**
 * @brief
 *
 * @param max_mem
 * @return int64_t
 */
int64_t LogEntryCache::Compact(int64_t max_mem) {
  // TODO:
  return -1;
}

/**
 * @brief return the memsize of all log entries
 *
 * @return uint64_t
 */
uint64_t LogEntryCache::MemSize() {
  return mem_size_;
}

/**
 * @brief return log entry count in log cache
 *
 * @return uint64_t
 */
uint64_t LogEntryCache::EntryCount() {
  return ent_count_;
}

/**
 * @brief return the first log index in log cache
 *
 * @return int64_t
 */
int64_t LogEntryCache::FirstIndex() {
  return first_ent_index_;
}
