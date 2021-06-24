#ifndef ERAFT_KV_PEERSTORAGE_H_
#define ERAFT_KV_PEERSTORAGE_H_

#include <iostream>
#include <eraftio/metapb.grpc.pb.h>
#include <eraftio/raft_serverpb.pb.h>

#include <Kv/Engines.h>
#include <RaftCore/Storage.h>

namespace kvserver
{

struct ApplySnapResult
{
    metapb::Region *prevRegion;

    metapb::Region *region;
};

class PeerStorage : public eraft::StorageInterface
{
public:

    PeerStorage(Engines *engs, metapb::Region *region, std::string tag);

    ~PeerStorage();

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

    std::shared_ptr<metapb::Region> region_;

    std::shared_ptr<raft_serverpb::RaftLocalState> raftState_;

    std::shared_ptr<raft_serverpb::RaftApplyState> applyState_;

    // TODO: snap

    uint64_t snapTriedCnt_;

    Engines *engines_;

    std::string tag_;
};

} // namespace kvserver

#endif