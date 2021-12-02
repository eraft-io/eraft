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

#ifndef ERAFT_RAFTCORE_STORAGE_H
#define ERAFT_RAFTCORE_STORAGE_H

#include <eraftio/eraftpb.pb.h>
#include <stdint.h>

#include <string>
#include <vector>

namespace eraft {

using EString = std::string;

// StorageInterface is an interface that may be implemented by the application
// to retrieve log entries from storage.
//
// If any Storage method returns an error, the raft instance will
// become inoperable and refuse to participate in elections; the
// application is responsible for cleanup and recovery in this case.
class StorageInterface {
 public:
  virtual ~StorageInterface() {}

  // InitialState returns the saved HardState and ConfState information.
  virtual std::pair<eraftpb::HardState, eraftpb::ConfState> InitialState() = 0;

  // Entries returns a slice of log entries in the range [lo,hi).
  // MaxSize limits the total size of the log entries returned, but
  // Entries returns at least one entry if any.
  virtual std::vector<eraftpb::Entry> Entries(uint64_t lo, uint64_t hi) = 0;

  // Term returns the term of entry i, which must be in the range
  // [FirstIndex()-1, LastIndex()]. The term of the entry before
  // FirstIndex is retained for matching purposes even though the
  // rest of that entry may not be available.
  virtual std::pair<uint64_t, bool> Term(uint64_t i) = 0;

  // LastIndex returns the index of the last entry in the log.
  virtual uint64_t LastIndex() = 0;

  // FirstIndex returns the index of the first log entry that is
  // possibly available via Entries (older entries have been incorporated
  // into the latest Snapshot; if storage only contains the dummy entry the
  // first log entry is not available).
  virtual uint64_t FirstIndex() = 0;

  // Snapshot returns the most recent snapshot.
  // If snapshot is temporarily unavailable, it should return
  // ErrSnapshotTemporarilyUnavailable, so raft state machine could know that
  // Storage needs some time to prepare snapshot and call Snapshot later.
  virtual eraftpb::Snapshot Snapshot() = 0;
};

}  // namespace eraft

#endif  // ERAFT_RAFTCORE_STORAGE_H