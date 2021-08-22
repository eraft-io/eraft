// MIT License

// Copyright (c) 2021 eraft dev group

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

#include <Kv/peer_storage.h>
#include <Kv/utils.h>
#include <Logger/logger.h>
#include <RaftCore/util.h>

namespace kvserver {

//
// init peer storage
//
PeerStorage::PeerStorage(std::shared_ptr<Engines> engs,
                         std::shared_ptr<metapb::Region> region,
                         std::string tag)
    : engines_(engs), region_(region), tag_(tag) {
  Logger::GetInstance()->DEBUG_NEW(
      "createing peer storage for region " + std::to_string(region->id()),
      __FILE__, __LINE__, "PeerStorage::PeerStorage");

  auto raftStatePair =
      Assistant::GetInstance()->InitRaftLocalState(engs->raftDB_, region);
  auto applyStatePair =
      Assistant::GetInstance()->InitApplyState(engs->kvDB_, region);

  if (raftStatePair.first->last_index() <
      applyStatePair.first->applied_index()) {
    Logger::GetInstance()->DEBUG_NEW(
        "err: raft log last index less than applied index! " +
            std::to_string(region->id()),
        __FILE__, __LINE__, "PeerStorage::PeerStorage");
    exit(-1);
  }
  this->raftState_ = raftStatePair.first;
  this->applyState_ = applyStatePair.first;
}

PeerStorage::~PeerStorage() {}

//
// InitialState implements the Storage interface.
// for init the peer state
//
std::pair<eraftpb::HardState, eraftpb::ConfState> PeerStorage::InitialState() {
  if (eraft::IsEmptyHardState(this->raftState_->hard_state())) {
    Logger::GetInstance()->DEBUG_NEW(
        "init peerstorage state with commit 5 and term 5 ", __FILE__, __LINE__,
        "PeerStorage::InitialState");
    this->raftState_->mutable_hard_state()->set_commit(5);
    this->raftState_->mutable_hard_state()->set_term(5);
    return std::pair<eraftpb::HardState, eraftpb::ConfState>(
        this->raftState_->hard_state(),
        Assistant::GetInstance()->ConfStateFromRegion(this->region_));
  }
  return std::pair<eraftpb::HardState, eraftpb::ConfState>(
      this->raftState_->hard_state(),
      Assistant::GetInstance()->ConfStateFromRegion(this->region_));
}

//
// Entries implements the Storage interface.
// return logs from log index in index [lo, hi)
//
std::vector<eraftpb::Entry> PeerStorage::Entries(uint64_t lo, uint64_t hi) {
  std::vector<eraftpb::Entry> ents;

  std::string startKey =
      Assistant::GetInstance()->RaftLogKey(this->region_->id(), lo);
  std::string endKey =
      Assistant::GetInstance()->RaftLogKey(this->region_->id(), hi);

  uint64_t nextIndex = lo;

  auto iter = this->engines_->raftDB_->NewIterator(rocksdb::ReadOptions());
  for (iter->Seek(startKey); iter->Valid(); iter->Next()) {
    if (Assistant::GetInstance()->ExceedEndKey(iter->key().ToString(),
                                               endKey)) {
      break;
    }
    std::string val = iter->value().ToString();
    eraftpb::Entry ent;
    ent.ParseFromString(val);

    if (ent.index() != nextIndex) {
      break;
    }

    nextIndex++;
    ents.push_back(ent);
  }
  return ents;
}

//
// Term implements the Storage interface.
// return the log term for log index = idx
//
std::pair<uint64_t, bool> PeerStorage::Term(uint64_t idx) {
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
  eraftpb::Entry* entry;
  Assistant::GetInstance()->GetMeta(
      this->engines_->raftDB_,
      Assistant::GetInstance()->RaftLogKey(this->region_->id(), idx), entry);
  return std::make_pair<uint64_t, bool>(entry->term(), true);
}

//
// LastIndex implements the Storage interface.
// return the last log index
//
uint64_t PeerStorage::LastIndex() { return this->raftState_->last_index(); }

//
// FirstIndex implements the Storage interface.
// return the first index
uint64_t PeerStorage::FirstIndex() { return this->TruncatedIndex() + 1; }

// Snapshot implements the Storage interface.
eraftpb::Snapshot PeerStorage::Snapshot() {
  std::shared_ptr<eraftpb::Snapshot> snap =
      std::make_shared<eraftpb::Snapshot>();
  snap->mutable_metadata()->set_index(
      this->applyState_->truncated_state().index());
  snap->mutable_metadata()->set_term(
      this->applyState_->truncated_state().term());
  return *snap;
}

//
// Append the new entries to storage.
// entries[0].Index > ms.entries[0].Index
//
bool PeerStorage::Append(std::vector<eraftpb::Entry> entries,
                         std::shared_ptr<rocksdb::WriteBatch> raftWB) {
  Logger::GetInstance()->DEBUG_NEW(
      "append " + std::to_string(entries.size()) + " to peerstorage", __FILE__,
      __LINE__, "PeerStorage::Append");
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
    Assistant::GetInstance()->SetMeta(
        raftWB.get(),
        Assistant::GetInstance()->RaftLogKey(regionId, entry.index()), entry);
  }

  uint64_t prevLast = this->LastIndex();
  if (prevLast > last) {
    for (uint64_t i = last + 1; i <= prevLast; i++) {
      raftWB->Delete(Assistant::GetInstance()->RaftLogKey(regionId, i));
    }
  }

  this->raftState_->set_last_index(last);
  this->raftState_->set_last_term(entries[entries.size() - 1].term());

  return true;
}

//
// return the applied index in current log
//
uint64_t PeerStorage::AppliedIndex() {
  return this->applyState_->applied_index();
}

//
// check if peer's region is initialized
//
bool PeerStorage::IsInitialized() {
  return (this->region_->peers().size() > 0);
}

//
// return this peerstorage's region
//
std::shared_ptr<metapb::Region> PeerStorage::Region() { return this->region_; }

void PeerStorage::SetRegion(std::shared_ptr<metapb::Region> region) {
  this->region_ = region;
}

//
// check if range [log, high] log entries is in peerstorge
//
bool PeerStorage::CheckRange(uint64_t low, uint64_t high) {
  if (low > high) {
    return false;
  } else if (low <= this->TruncatedIndex()) {
    return false;
  } else if (high > this->raftState_->last_index() + 1) {
    return false;
  }
  return true;
}

uint64_t PeerStorage::TruncatedIndex() {
  return this->applyState_->truncated_state().index();
}

uint64_t PeerStorage::TruncatedTerm() {
  return this->applyState_->truncated_state().term();
}

bool PeerStorage::ValidateSnap(std::shared_ptr<eraftpb::Snapshot> snap) {
  // TODO: check snap
}

bool PeerStorage::ClearMeta(std::shared_ptr<rocksdb::WriteBatch> kvWB,
                            std::shared_ptr<rocksdb::WriteBatch> raftWB) {
  return Assistant::GetInstance()->DoClearMeta(
      this->engines_, kvWB.get(), raftWB.get(), this->region_->id(),
      this->raftState_->last_index());
}

//
// save memory states to disk
//
std::shared_ptr<ApplySnapResult> PeerStorage::SaveReadyState(
    std::shared_ptr<eraft::DReady> ready) {
  std::shared_ptr<rocksdb::WriteBatch> raftWB =
      std::make_shared<rocksdb::WriteBatch>();
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

  this->Append(ready->entries, raftWB);

  // if (eraft::IsEmptyHardState(ready->hardSt)) {
  this->raftState_->mutable_hard_state()->set_commit(ready->hardSt.commit());
  this->raftState_->mutable_hard_state()->set_term(ready->hardSt.term());
  this->raftState_->mutable_hard_state()->set_vote(ready->hardSt.vote());
  // }
  Assistant::GetInstance()->SetMeta(
      raftWB.get(), Assistant::GetInstance()->RaftStateKey(this->region_->id()),
      *this->raftState_);
  this->engines_->raftDB_->Write(rocksdb::WriteOptions(), raftWB.get());
  return std::make_shared<ApplySnapResult>(result);
}

void PeerStorage::ClearData() {
  this->ClearRange(this->region_->id(), this->region_->start_key(),
                   this->region_->end_key());
}

void PeerStorage::ClearRange(uint64_t regionID, std::string start,
                             std::string end) {
  // sched region destory task
}

}  // namespace kvserver