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
 * @file log_entry_cache.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-05-21
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

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
  /**
   * @brief
   *
   */
  std::deque<eraftkv::Entry*> entries_;

  /**
   * @brief
   *
   */
  int64_t first_ent_index_;

  /**
   * @brief
   *
   */
  uint64_t mem_size_;

  /**
   * @brief
   *
   */
  uint64_t ent_count_;
};
