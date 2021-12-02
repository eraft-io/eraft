// Copyright 2015 The etcd Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//
// MIT License

// Copyright (c) 2021 Colin

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

#ifndef ERAFT_RAFTCORE_LOG_H_
#define ERAFT_RAFTCORE_LOG_H_

#include <raftcore/memory_storage.h>
#include <stdint.h>

// RaftLog manage the log entries, its struct look like:
//
//  snapshot/first.....applied....committed....stabled.....last
//  --------|------------------------------------------------|
//                            log entries
//
// for simplify the RaftLog implement should manage all log entries
// that not truncated

namespace eraft {

enum class NodeState {
  StateFollower,
  StateCandidate,
  StateLeader,
};

class RaftLog {
 public:
  friend class RaftContext;

  friend class RawNode;

  // RaftLog(StorageInterface &st);
  RaftLog(std::shared_ptr<StorageInterface> st);

  ~RaftLog();

  // LastIndex return the last index of the log entries
  uint64_t LastIndex();

  // Term return the term of the entry in the given index
  std::pair<uint64_t, bool> Term(uint64_t i);

  // We need to compact the log entries in some point of time like
  // storage compact stabled log entries prevent the log entries
  // grow unlimitedly in memory
  void MaybeCompact();

  // unstableEntries return all the unstable entries
  std::vector<eraftpb::Entry> UnstableEntries();

  // nextEnts returns all the committed but not applied entries
  std::vector<eraftpb::Entry> NextEnts();

  uint64_t ToSliceIndex(uint64_t i);

  uint64_t ToEntryIndex(uint64_t i);

  // committed is the highest log position that is known to be in
  // stable storage on a quorum of nodes.
  uint64_t commited_;

  // applied is the highest log position that the application has
  // been instructed to apply to its state machine.
  // Invariant: applied <= committed
  uint64_t applied_;

  // log entries with index <= stabled are persisted to storage.
  // It is used to record the logs that are not persisted by storage yet.
  // Everytime handling `Ready`, the unstabled logs will be included.
  uint64_t stabled_;

  uint64_t firstIndex_;

  // all entries that have not yet compact.
  std::vector<eraftpb::Entry> entries_;

 private:
  // storage contains all stable entries since the last snapshot.
  std::shared_ptr<StorageInterface> storage_;  // point to real storage

  // the incoming unstable snapshot, if any.
  eraftpb::Snapshot pendingSnapshot_;
};

}  // namespace eraft

#endif  // ERAFT_RAFTCORE_LOG_H_