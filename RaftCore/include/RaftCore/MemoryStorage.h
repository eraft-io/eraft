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

// @file MemoryStorage.h
// @author Colin
// This module declares the eraft::MemoryStorage class.
// 
// Inspired by etcd golang version.

#ifndef ERAFT_MEMORYSTORAGE_H
#define ERAFT_MEMORYSTORAGE_H

#include "Storage.h"
#include <mutex>

namespace eraft
{

class MemoryStorage : public StorageInterface {
    
public:

    MemoryStorage();

    // InitialState implements the Storage interface.
    std::tuple<eraftpb::HardState, eraftpb::ConfState> InitialState() override;

    // Entries implements the Storage interface.
    std::vector<eraftpb::Entry> Entries(uint64_t lo, uint64_t hi) override;

    // Term implements the Storage interface.
    uint64_t Term(uint64_t i) override;

    // LastIndex implements the Storage interface.
    uint64_t LastIndex() override;

    // FirstIndex implements the Storage interface.
    uint64_t FirstIndex() override;

    // Snapshot implements the Storage interface.
    eraftpb::Snapshot Snapshot() override;

    // SetHardState saves the current HardState.
    void SetHardState(eraftpb::HardState &st);

    // ApplySnapshot overwrites the contents of this Storage object with
    // those of the given snapshot.
    bool ApplySnapshot(eraftpb::Snapshot &snap);

    // CreateSnapshot makes a snapshot which can be retrieved with Snapshot() and
    // can be used to reconstruct the state at that point.
    // If any configuration changes have been made since the last compaction,
    // the result of the last ApplyConfChange must be passed in.
    eraftpb::Snapshot CreateSnapshot(uint64_t i, eraftpb::ConfState* cs, const char* bytes);

    // Compact discards all log entries prior to compactIndex.
    // It is the application's responsibility to not attempt to compact an index
    // greater than raftLog.applied.
    bool Compact(uint64_t compactIndex);

    // Append the new entries to storage.
    // entries[0].Index > ms.entries[0].Index
    bool Append(std::vector<eraftpb::Entry> entries);

private:

    std::mutex mutex_;

    eraftpb::HardState hardState_;

    eraftpb::Snapshot snapShot_;

    std::vector<eraftpb::Entry> ents_;

};
    
} // namespace eraft


#endif