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

#ifndef ERAFT_KV_PEERSTORAGE_H_
#define ERAFT_KV_PEERSTORAGE_H_

#include <Kv/engines.h>
#include <RaftCore/raw_node.h>
#include <RaftCore/storage.h>
#include <eraftio/metapb.pb.h>
#include <eraftio/raft_serverpb.pb.h>
#include <rocksdb/write_batch.h>
#include <stdint.h>

#include <iostream>
#include <memory>

namespace kvserver {

class ApplySnapResult {
 public:
  std::shared_ptr<metapb::Region> prevRegion;

  std::shared_ptr<metapb::Region> region;

  ApplySnapResult() {}

  ~ApplySnapResult() {}
};

class PeerStorage : public eraft::StorageInterface {
 public:
  PeerStorage(std::shared_ptr<Engines> engs,
              std::shared_ptr<metapb::Region> region, std::string tag);

  ~PeerStorage();

  // InitialState implements the Storage interface.
  std::pair<eraftpb::HardState, eraftpb::ConfState> InitialState() override;

  // Entries implements the Storage interface.
  std::vector<eraftpb::Entry> Entries(uint64_t lo, uint64_t hi) override;

  // Term implements the Storage interface.
  std::pair<uint64_t, bool> Term(uint64_t i) override;

  // LastIndex implements the Storage interface.
  uint64_t LastIndex() override;

  // FirstIndex implements the Storage interface.
  uint64_t FirstIndex() override;

  uint64_t AppliedIndex();

  // Snapshot implements the Storage interface.
  eraftpb::Snapshot Snapshot() override;

  bool IsInitialized();

  std::shared_ptr<metapb::Region> Region();

  void SetRegion(std::shared_ptr<metapb::Region> region);

  bool CheckRange(uint64_t low, uint64_t high);

  uint64_t TruncatedIndex();

  uint64_t TruncatedTerm();

  bool ValidateSnap(std::shared_ptr<eraftpb::Snapshot> snap);

  bool ClearMeta(std::shared_ptr<rocksdb::WriteBatch> kvWB,
                 std::shared_ptr<rocksdb::WriteBatch> raftWB);

  std::shared_ptr<ApplySnapResult> SaveReadyState(
      std::shared_ptr<eraft::DReady> ready);

  void ClearData();

  void ClearRange(uint64_t regionID, std::string start, std::string end);

  bool Append(std::vector<eraftpb::Entry> entries,
              std::shared_ptr<rocksdb::WriteBatch> raftWB);

  std::shared_ptr<metapb::Region> region_;

  raft_serverpb::RaftLocalState* raftState_;

  raft_serverpb::RaftApplyState* applyState_;

  std::shared_ptr<Engines> engines_;

 private:
  std::string tag_;
};

}  // namespace kvserver

#endif