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

// @file Storage.h
// @author Colin
// This module declares the eraft::StorageInterface interface.
//
// Inspired by etcd golang version.

#ifndef ERAFT_STORAGE_H
#define ERAFT_STORAGE_H


#include <stdint.h>
#include <string>
#include <vector>
#include <eraftio/eraftpb.pb.h>

namespace eraft
{

using EString = std::string;

// StorageInterface is an interface that may be implemented by the application
// to retrieve log entries from storage.
//
// If any Storage method returns an error, the raft instance will
// become inoperable and refuse to participate in elections; the
// application is responsible for cleanup and recovery in this case.
class StorageInterface
{
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
    virtual uint64_t Term(uint64_t i) = 0;

	// LastIndex returns the index of the last entry in the log.
    virtual uint64_t LastIndex() = 0;

	// FirstIndex returns the index of the first log entry that is
	// possibly available via Entries (older entries have been incorporated
	// into the latest Snapshot; if storage only contains the dummy entry the
	// first log entry is not available).
    virtual uint64_t FirstIndex() = 0;

	// Snapshot returns the most recent snapshot.
	// If snapshot is temporarily unavailable, it should return ErrSnapshotTemporarilyUnavailable,
	// so raft state machine could know that Storage needs some time to prepare
	// snapshot and call Snapshot later.
    virtual eraftpb::Snapshot Snapshot() = 0;

	virtual bool Append(std::vector<eraftpb::Entry> entries) = 0;

};

} // namespace name


#endif
