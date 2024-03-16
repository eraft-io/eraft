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
 * @file raft_log.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-05-21
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <string>
#include <vector>

#include "eraftkv.pb.h"
#include "estatus.h"
/**
 * @brief
 *
 */
class LogStore {
 public:
  /**
   * @brief Destroy the Log Store object
   *
   */
  virtual ~LogStore() {}

  virtual EStatus Reinit() = 0;

  /**
   * @brief Append add new entries
   *
   * @param ety
   * @return EStatus
   */
  virtual EStatus Append(eraftkv::Entry* ety) = 0;

  /**
   * @brief EraseBefore erase all entries before the given index
   *
   * @param first_index
   * @return EStatus
   */
  virtual EStatus EraseBefore(int64_t first_index) = 0;

  /**
   * @brief EraseAfter erase all entries after the given index
   *
   * @param from_index
   * @return EStatus
   */
  virtual EStatus EraseAfter(int64_t from_index) = 0;

  /**
   * @brief Get get the given index entry
   *
   * @param index
   * @return eraftkv::Entry*
   */
  virtual eraftkv::Entry* Get(int64_t index) = 0;

  /**
   * @brief Gets get the given index range entry
   *
   * @param start_index
   * @param end_index
   * @return std::vector<eraftkv::Entry*>
   */
  virtual std::vector<eraftkv::Entry*> Gets(int64_t start_index,
                                            int64_t end_index) = 0;

  /**
   * @brief FirstIndex get the first index in the entry
   *
   * @return int64_t
   */
  virtual int64_t FirstIndex() = 0;

  /**
   * @brief LastIndex get the last index in the entry
   *
   * @return int64_t
   */
  virtual int64_t LastIndex() = 0;

  /**
   * @brief
   *
   * @param new_idx
   * @return true
   * @return false
   */
  virtual void ResetFirstIndex(int64_t new_idx) = 0;

  /**
   * @brief
   *
   * @param term
   * @param index
   */
  virtual void ResetFirstLogEntry(int64_t term, int64_t index) = 0;

  /**
   * @brief LogCount get the number of entries
   *
   * @return int64_t
   */
  virtual int64_t LogCount() = 0;

  /**
   * @brief Get the First Ety object
   *
   * @return eraftkv::Entry*
   */
  virtual eraftkv::Entry* GetFirstEty() = 0;

  /**
   * @brief Get the Last Ety object
   *
   * @return eraftkv::Entry*
   */
  virtual eraftkv::Entry* GetLastEty() = 0;

  virtual EStatus PersisLogMetaState(int64_t commit_idx,
                                     int64_t applied_idx) = 0;

  virtual EStatus ReadMetaState(int64_t* commit_idx, int64_t* applied_idx) = 0;
};
