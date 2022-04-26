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

#include <network/raft_encode_assistant.h>
#include <network/raft_peer_storage.h>
#include <raftcore/util.h>

#include <memory>

namespace network {

RaftPeerStorage::RaftPeerStorage(
    std::shared_ptr<storage::StorageEngineInterface> engs,
    std::shared_ptr<metapb::Region> region, std::string tag)
    : engines_(engs), region_(region), tag_(tag) {
  SPDLOG_INFO("createing peer storage for region " +
              std::to_string(region->id()));
  auto raftStatePair = RaftEncodeAssistant::GetInstance()->InitRaftLocalState(
      engs->raftDB_, region);

  auto applyStatePair =
      RaftEncodeAssistant()->GetInstance()->InitApplyState(engs->kvDB_, region);

  // checkout last index < applied_index
  this->raftState_ = raftStatePair.first;
  this->applyState_ = applyStatePair.first;
}

RaftPeerStorage::~RaftPeerStorage() {}

// InitialState returns the saved HardState and ConfState information.
std::pair<eraftpb::HardState, eraftpb::ConfState>
RaftPeerStorage::InitialState() {
  if (eraft::IsEmptyHardState(this->raftState_->hard_state())) {
    SPDLOG_INFO("init peerstorage state with commit 5 and term 5 ");
    this->raftState_->mutable_hard_state()->set_commit(5);
    this->raftState_->mutable_hard_state()->set_term(5);
    return std::pair<eraftpb::HardState, eraftpb::ConfState>(
        this->raftState_->hard_state(),
        RaftEncodeAssistant::GetInstance()->ConfStateFromRegion(this->region_));
  }
  return std::pair<eraftpb::HardState, eraftpb::ConfState>(
      this->raftState_->hard_state(),
      RaftEncodeAssistant::GetInstance()->ConfStateFromRegion(this->region_));
}

// Entries returns a slice of log entries in the range [lo,hi).
// MaxSize limits the total size of the log entries returned, but
// Entries returns at least one entry if any.
std::vector<eraftpb::Entry> RaftPeerStorage::Entries(uint64_t lo, uint64_t hi) {
  // pmem range query
}

// Term returns the term of entry i, which must be in the range
// [FirstIndex()-1, LastIndex()]. The term of the entry before
// FirstIndex is retained for matching purposes even though the
// rest of that entry may not be available.
std::pair<uint64_t, bool> RaftPeerStorage::Term(uint64_t idx) {
  if (idx == this->TruncatedIndex()) {
    return std::make_pair<uint64_t, bool>(this->TruncatedTerm(), true);
  }
  // TODO: check idx, idx+1
  if (!this->CheckRange(idx, idx + 1)) {
    return std::make_pair<uint64_t, bool>(0, false);
  }
  if (this->TruncatedTerm() == this->raftState_->last_term() ||
      idx == this->raftState_->last_index()) {
    return std::make_pair<uint64_t, bool>(this->raftState_->last_term(), true);
  }
  eraftpb::Entry *entry;
  RaftEncodeAssistant *assistant = RaftEncodeAssistant::GetInstance();
  assistant->GetMessageFromEngine(
      this->engines_->raftDB_, assistant->RaftLogKey(this->region_->id(), idx),
      entry);

  return std::make_pair<uint64_t, bool>(entry->term(), true);
}

// LastIndex returns the index of the last entry in the log.
uint64_t RaftPeerStorage::LastIndex() { return this->raftState_->last_index(); }

// FirstIndex returns the index of the first log entry that is
// possibly available via Entries (older entries have been incorporated
// into the latest Snapshot; if storage only contains the dummy entry the
// first log entry is not available).
uint64_t RaftPeerStorage::FirstIndex() { return this->TruncatedIndex() + 1; }

// Snapshot returns the most recent snapshot.
// If snapshot is temporarily unavailable, it should return
// ErrSnapshotTemporarilyUnavailable, so raft state machine could know that
// Storage needs some time to prepare snapshot and call Snapshot later.
eraftpb::Snapshot RaftPeerStorage::Snapshot() {}

bool RaftPeerStorage::IsInitialized() {
  return (this->region_->peers().size() > 0);
}

std::shared_ptr<metapb::Region> RaftPeerStorage::Region() {}

void RaftPeerStorage::SetRegion(std::shared_ptr<metapb::Region> region) {
  this->region_ = region;
}

bool RaftPeerStorage::CheckRange(uint64_t low, uint64_t high) {}

uint64_t RaftPeerStorage::AppliedIndex() {
  return this->applyState_->applied_index();
}

uint64_t RaftPeerStorage::TruncatedIndex() {
  return this->applyState_->truncated_state().index();
}

uint64_t RaftPeerStorage::TruncatedTerm() {
  return this->applyState_->truncated_state().term();
}

bool RaftPeerStorage::ClearMeta() {}

std::shared_ptr<ApplySnapResult> RaftPeerStorage::SaveReadyState(
    std::shared_ptr<eraft::DReady> ready) {
  std::shared_ptr<storage::WriteBatch> raftWB =
      std::make_shared<storage::WriteBatch>();

  ApplySnapResult result;
  if (!eraft::IsEmptySnap(ready->snapshot)) {
    this->raftState_->set_last_index(ready->snapshot.metadata().index());
    this->raftState_->set_last_term(ready->snapshot.metadata().term());
    this->applyState_->set_applied_index(ready->snapshot.metadata().index());
    this->applyState_->mutable_truncated_state()->set_index(
        ready->snapshot.metadata().index());
    this->applyState_->mutable_truncated_state()->set_term(
        ready->snapshot.metadata().term());
  }

  this->Append(ready->entries, this->engines_->raftDB_);

  this->raftState_->mutable_hard_state()->set_commit(ready->hardSt.commit());
  this->raftState_->mutable_hard_state()->set_term(ready->hardSt.term());
  this->raftState_->mutable_hard_state()->set_vote(ready->hardSt.vote());

  RaftEncodeAssistant::GetInstance()->PutMessageToEngine(
      this->engines_->raftDB_,
      RaftEncodeAssistant::GetInstance()->RaftStateKey(this->region_->id()),
      *this->raftState_);

  return std::make_shared<ApplySnapResult>(result);
}

void RaftPeerStorage::ClearRange(uint64_t regionID, std::string start,
                                 std::string end) {}

bool RaftPeerStorage::Append(
    std::vector<eraftpb::Entry> entries,
    std::shared_ptr<storage::StorageEngineInterface> raftEng) {
  SPDLOG_INFO("append " + std::to_string(entries.size()) + " to peerstorage");

  if (entries.size() == 0) {
    return false;
  }

  uint64_t first = this->FirstIndex();
  uint64_t last = entries[entries.size() - 1].index();

  if (last < first) {
    return false;
  }

  if (first > entries[0].index()) {
    entries.erase(entries.begin() + (first - entries[0].index()));
  }

  uint64_t regionId = this->region_->id();
  for (auto entry : entries) {
    RaftEncodeAssistant::GetInstance()->PutMessageToEngine(
        raftEng,
        RaftEncodeAssistant::GetInstance()->RaftLogKey(regionId, entry.index()),
        entry);
  }

  uint64_t prevLast = this->LastIndex();
  if (prevLast > last) {
    for (uint64_t i = last + 1; i <= prevLast; i++) {
      raftEng->RemoveK(
          RaftEncodeAssistant::GetInstance()->RaftLogKey(regionId, i));
    }
  }

  this->raftState_->set_last_index(last);
  this->raftState_->set_last_term(entries[entries.size() - 1].term());

  return true;
}

std::shared_ptr<metapb::Region> RaftPeerStorage::GetRegion() {
  return this->region_;
}

raft_messagepb::RaftLocalState *RaftPeerStorage::GetRaftLocalState() {
  return this->raftState_;
}

raft_messagepb::RaftApplyState *RaftPeerStorage::GetRaftApplyState() {
  return this->applyState_;
}

}  // namespace network
