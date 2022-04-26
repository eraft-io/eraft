// MIT License

// Copyright (c) 2022 eraft dev group

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

#ifndef ERAFT_RAFT_PEER_STORAGE_H_
#define ERAFT_RAFT_PEER_STORAGE_H_

#include <eraftio/metapb.pb.h>
#include <eraftio/raft_messagepb.pb.h>
#include <raftcore/raw_node.h>
#include <raftcore/storage.h>

#include <cstdint>
#include <iostream>
#include <memory>

namespace network {

class ApplySnapResult {
 public:
  std::shared_ptr<metapb::Region> prevRegion;

  std::shared_ptr<metapb::Region> region;

  ApplySnapResult() {}

  ~ApplySnapResult() {}
};

class RaftPeerStorage : public eraft::StorageInterface {
 public:
  RaftPeerStorage(std::shared_ptr<storage::StorageEngineInterface> engs,
                  std::shared_ptr<metapb::Region> region, std::string tag);

  ~RaftPeerStorage();

  // InitialState returns the saved HardState and ConfState information.
  std::pair<eraftpb::HardState, eraftpb::ConfState> InitialState() override;

  // Entries returns a slice of log entries in the range [lo,hi).
  // MaxSize limits the total size of the log entries returned, but
  // Entries returns at least one entry if any.
  std::vector<eraftpb::Entry> Entries(uint64_t lo, uint64_t hi) override;

  // Term returns the term of entry i, which must be in the range
  // [FirstIndex()-1, LastIndex()]. The term of the entry before
  // FirstIndex is retained for matching purposes even though the
  // rest of that entry may not be available.
  std::pair<uint64_t, bool> Term(uint64_t i) override;

  // LastIndex returns the index of the last entry in the log.
  uint64_t LastIndex() override;

  uint64_t AppliedIndex();

  // FirstIndex returns the index of the first log entry that is
  // possibly available via Entries (older entries have been incorporated
  // into the latest Snapshot; if storage only contains the dummy entry the
  // first log entry is not available).
  uint64_t FirstIndex() override;

  // Snapshot returns the most recent snapshot.
  // If snapshot is temporarily unavailable, it should return
  // ErrSnapshotTemporarilyUnavailable, so raft state machine could know that
  // Storage needs some time to prepare snapshot and call Snapshot later.
  eraftpb::Snapshot Snapshot() override;

  bool IsInitialized();

  std::shared_ptr<metapb::Region> Region();

  void SetRegion(std::shared_ptr<metapb::Region> region);

  bool CheckRange(uint64_t low, uint64_t high);

  uint64_t TruncatedIndex();

  uint64_t TruncatedTerm();

  bool ClearMeta();

  std::shared_ptr<ApplySnapResult> SaveReadyState(
      std::shared_ptr<eraft::DReady> ready);

  void ClearRange(uint64_t regionID, std::string start, std::string end);

  bool Append(std::vector<eraftpb::Entry> entries,
              std::shared_ptr<storage::StorageEngineInterface> raftEng);

  std::shared_ptr<metapb::Region> GetRegion();

  raft_messagepb::RaftLocalState *GetRaftLocalState();

  raft_messagepb::RaftApplyState *GetRaftApplyState();

 private:
  std::string tag_;

  std::shared_ptr<metapb::Region> region_;

  raft_messagepb::RaftLocalState *raftState_;

  raft_messagepb::RaftApplyState *applyState_;

  std::shared_ptr<storage::StorageEngineInterface> engines_;
};

}  // namespace network

#endif
