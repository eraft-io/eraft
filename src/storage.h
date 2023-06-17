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
 * @file storage.h
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

#include "estatus.h"
#include "raft_server.h"

class RaftServer;
/**
 * @brief
 *
 */
class Storage {

 public:
  /**
   * @brief Destroy the Storage object
   *
   */
  virtual ~Storage() {}


  /**
   * @brief Get the Node Address object
   *
   * @param raft
   * @param id
   * @return std::string
   */
  virtual std::string GetNodeAddress(RaftServer* raft, std::string id) = 0;

  /**
   * @brief
   *
   * @param raft
   * @param id
   * @param address
   * @return EStatus
   */
  virtual EStatus SaveNodeAddress(RaftServer* raft,
                                  std::string id,
                                  std::string address) = 0;

  /**
   * @brief
   *
   * @param raft
   * @param snapshot_index
   * @param snapshot_term
   * @return EStatus
   */
  virtual EStatus ApplyLog(RaftServer* raft,
                           int64_t     snapshot_index,
                           int64_t     snapshot_term) = 0;

  /**
   * @brief Get the Snapshot Block object
   *
   * @param raft
   * @param node
   * @param offset
   * @param block
   * @return EStatus
   */
  virtual EStatus GetSnapshotBlock(RaftServer*             raft,
                                   RaftNode*               node,
                                   int64_t                 offset,
                                   eraftkv::SnapshotBlock* block) = 0;

  /**
   * @brief
   *
   * @param raft
   * @param snapshot_index
   * @param offset
   * @param block
   * @return EStatus
   */
  virtual EStatus StoreSnapshotBlock(RaftServer*             raft,
                                     int64_t                 snapshot_index,
                                     int64_t                 offset,
                                     eraftkv::SnapshotBlock* block) = 0;

  /**
   * @brief
   *
   * @param raft
   * @return EStatus
   */
  virtual EStatus ClearSnapshot(RaftServer* raft) = 0;

  /**
   * @brief
   *
   * @return EStatus
   */
  virtual EStatus CreateDBSnapshot() = 0;

  /**
   * @brief
   *
   * @param raft
   * @param term
   * @param vote
   * @return EStatus
   */
  virtual EStatus SaveRaftMeta(RaftServer* raft,
                               int64_t     term,
                               int64_t     vote) = 0;

  /**
   * @brief
   *
   * @param raft
   * @param term
   * @param vote
   * @return EStatus
   */
  virtual EStatus ReadRaftMeta(RaftServer* raft,
                               int64_t*    term,
                               int64_t*    vote) = 0;

  /**
   * @brief
   *
   * @param key
   * @param val
   * @return EStatus
   */
  virtual EStatus PutKV(std::string key, std::string val) = 0;

  /**
   * @brief
   *
   * @param key
   * @return std::string
   */
  virtual std::string GetKV(std::string key) = 0;
};
