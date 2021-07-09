#ifndef ERAFT_KV_PEERSTORAGE_H_
#define ERAFT_KV_PEERSTORAGE_H_

#include <iostream>
#include <memory>

#include <eraftio/metapb.pb.h>
#include <eraftio/raft_serverpb.pb.h>

#include <Kv/engines.h>
#include <RaftCore/Storage.h>
#include <RaftCore/RawNode.h>
#include <rocksdb/write_batch.h>

namespace kvserver
{

class ApplySnapResult
{
    public:

    std::shared_ptr<metapb::Region> prevRegion;

    std::shared_ptr<metapb::Region> region;

    ApplySnapResult() {}

    ~ApplySnapResult() {}
};

class PeerStorage : public eraft::StorageInterface
{
public:

    PeerStorage(std::shared_ptr<Engines> engs, std::shared_ptr<metapb::Region> region, std::string tag);

    ~PeerStorage();

    // InitialState implements the Storage interface.
    std::pair<eraftpb::HardState, eraftpb::ConfState> InitialState() override;

    // Entries implements the Storage interface.
    std::vector<eraftpb::Entry> Entries(uint64_t lo, uint64_t hi) override;

    // Term implements the Storage interface.
    uint64_t Term(uint64_t i) override;

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

    bool ClearMeta(std::shared_ptr<rocksdb::WriteBatch> kvWB, std::shared_ptr<rocksdb::WriteBatch> raftWB);

    void ClearExtraData(std::shared_ptr<metapb::Region> newRegion);

    std::shared_ptr<ApplySnapResult> SaveReadyState(std::shared_ptr<eraft::DReady> ready);

    void ClearData();

    void ClearRange(uint64_t regionID, std::string start, std::string end);    

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
    bool Append(std::vector<eraftpb::Entry> entries, std::shared_ptr<rocksdb::WriteBatch> raftWB);

    std::shared_ptr<metapb::Region> region_;

private:

    raft_serverpb::RaftLocalState* raftState_;

    raft_serverpb::RaftApplyState* applyState_;

    // TODO: snap
    uint64_t snapTriedCnt_;

    std::shared_ptr<Engines> engines_;

    std::string tag_;
};

} // namespace kvserver

#endif