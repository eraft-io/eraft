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

#include <raftcore/memory_storage.h>
#include <string.h>

#include <iostream>
#include <tuple>

namespace eraft {

MemoryStorage::MemoryStorage() {
  this->ents_ = std::vector<eraftpb::Entry>{};
  this->ents_.resize(1);
  this->ents_[0].set_index(0);
  this->snapShot_ = eraftpb::Snapshot();
}

std::pair<eraftpb::HardState, eraftpb::ConfState>
MemoryStorage::InitialState() {
  return std::make_pair(this->hardState_,
                        this->snapShot_.mutable_metadata()->conf_state());
}

void MemoryStorage::SetHardState(eraftpb::HardState& st) {
  std::lock_guard<std::mutex> lck(mutex_);
  this->hardState_ = st;
}

std::vector<eraftpb::Entry> MemoryStorage::Entries(uint64_t lo, uint64_t hi) {
  std::vector<eraftpb::Entry> ents;
  uint64_t offset = this->ents_[0].index();
  if (lo <= offset) {
    return ents;
  }
  if (hi > this->LastIndex() + 1) {
    // TODO: log panic ["entries' hi(%d) is out of bound lastindex(%d)", hi,
    // ms.lastIndex()]
  }
  {
    std::lock_guard<std::mutex> lck(mutex_);
    std::vector<eraftpb::Entry> ents2;
    ents2.insert(ents2.begin(), this->ents_.begin() + (lo - offset),
                 this->ents_.begin() + (hi - offset));
    ents = ents2;
    if (this->ents_.size() == 1 && ents.size() != 0) {
      return std::vector<eraftpb::Entry>{};
    }
  }
  return ents;
}

std::pair<uint64_t, bool> MemoryStorage::Term(uint64_t i) {
  // std::lock_guard<std::mutex> lck (mutex_);
  uint64_t offset = this->ents_[0].index();
  if (i < offset) {
    return std::make_pair<uint64_t, bool>(0, true);
  }
  if ((i - offset) >= this->ents_.size()) {
    return std::make_pair<uint64_t, bool>(0, true);
  }
  return std::make_pair<uint64_t, bool>(this->ents_[i - offset].term(), true);
}

uint64_t MemoryStorage::LastIndex() {
  std::lock_guard<std::mutex> lck(mutex_);
  return (this->ents_[0].index() + this->ents_.size() - 1);
}

uint64_t MemoryStorage::FirstIndex() {
  std::lock_guard<std::mutex> lck(mutex_);
  return (this->ents_[0].index() + 1);
}

eraftpb::Snapshot MemoryStorage::Snapshot() {
  std::lock_guard<std::mutex> lck(mutex_);
  return this->snapShot_;
}

/**
 *
 * @return bool (is successful)
 */

bool MemoryStorage::ApplySnapshot(eraftpb::Snapshot& snap) {
  uint64_t msIndex = this->snapShot_.metadata().index();
  uint64_t snapIndex = snap.metadata().index();
  if (msIndex >= snapIndex) {
    // TODO: log err snap out of date
    return false;
  }
  this->snapShot_ = snap;
  eraftpb::Entry entry;
  {
    std::lock_guard<std::mutex> lck(mutex_);
    entry.set_term(snap.metadata().term());
    entry.set_index(snap.metadata().index());
    this->ents_.push_back(entry);
  }
  return true;
}

eraftpb::Snapshot MemoryStorage::CreateSnapshot(uint64_t i,
                                                eraftpb::ConfState* cs,
                                                const char* data) {
  {
    std::lock_guard<std::mutex> lck(mutex_);
    if (i < this->snapShot_.metadata().index()) {
      return eraftpb::Snapshot();
    }
    uint64_t offset = this->ents_[0].index();
    if (i > this->LastIndex()) {
      // TODO: log panic
    }
    eraftpb::SnapshotMetadata meta_;
    meta_.set_index(i);
    meta_.set_term(this->ents_[i - offset].term());
    if (cs != nullptr) {
      meta_.set_allocated_conf_state(cs);
    }
    this->snapShot_.set_data(data);
    this->snapShot_.set_allocated_metadata(&meta_);
  }
  return this->snapShot_;
}

bool MemoryStorage::Compact(uint64_t compactIndex) {
  std::lock_guard<std::mutex> lck(mutex_);
  uint64_t offset = this->ents_[0].index();
  if (compactIndex <= offset) {
    // log error compacted
    return false;
  }
  if (compactIndex > this->LastIndex()) {
    // log panic compact compactIndex is out of bound
    return false;
  }
  uint64_t i = compactIndex - offset;
  std::vector<eraftpb::Entry> ents;
  uint64_t newSize_ = 1 + this->ents_.size() - i;
  ents.resize(newSize_);
  for (uint64_t index = i; index < this->LastIndex(); index++) {
    ents.push_back(this->ents_[index]);
  }
  this->ents_ = ents;
  return true;
}

bool MemoryStorage::Append(std::vector<eraftpb::Entry> entries) {
  if (entries.size() == 0) {
    return false;
  }

  uint64_t first = this->FirstIndex();
  uint64_t last = entries[0].index() + entries.size() - 1;

  // std::cout << "first: " << first << " entries[0].index():" <<
  // entries[0].index() << " entries.size() " << entries.size() << " last " <<
  // last << std::endl; shortcut if there is no new entry.
  if (last < first) {
    return false;
  }
  if (first > entries[0].index()) {
    {
      std::lock_guard<std::mutex> lck(mutex_);
      entries.erase(entries.begin() + (first - entries[0].index() - 1));
    }
  }
  // offset = 1
  uint64_t offset = entries[0].index() - this->ents_[0].index();
  std::vector<eraftpb::Entry> ents;
  // std::cout << "this->ents_.size(): " << this->ents_.size() << " offset " <<
  // offset << std::endl;
  if (this->ents_.size() > offset) {
    {
      std::lock_guard<std::mutex> lck(mutex_);
      ents_ = this->ents_;
      ents.insert(ents.begin(), this->ents_.begin(),
                  this->ents_.begin() + offset);
      ents.insert(ents.end(), entries.begin(), entries.end());
      this->ents_ = ents;
    }
  } else if (this->ents_.size() == offset) {
    {
      std::lock_guard<std::mutex> lck(mutex_);
      for (auto e : entries) {
        this->ents_.push_back(e);
      }
      // std::cout << "this->ents_.size(): " << this->ents_.size() << std::endl;
    }
  } else {
    // TODO: log panic
  }
  return true;
}

}  // namespace eraft
