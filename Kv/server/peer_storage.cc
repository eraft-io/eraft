#include <Kv/peer_storage.h>

namespace kvserver {

PeerStorage::PeerStorage(Engines *engs, metapb::Region *region, std::string tag)
{

}

PeerStorage::~PeerStorage()
{

}

// InitialState implements the Storage interface.
std::tuple<eraftpb::HardState, eraftpb::ConfState> PeerStorage::InitialState()
{

}

// Entries implements the Storage interface.
std::vector<eraftpb::Entry> PeerStorage::Entries(uint64_t lo, uint64_t hi)
{

}

// Term implements the Storage interface.
uint64_t PeerStorage::Term(uint64_t i)
{

}

// LastIndex implements the Storage interface.
uint64_t PeerStorage::LastIndex()
{

}

// FirstIndex implements the Storage interface.
uint64_t PeerStorage::FirstIndex()
{

}

// Snapshot implements the Storage interface.
eraftpb::Snapshot PeerStorage::Snapshot()
{

}

// SetHardState saves the current HardState.
void PeerStorage::SetHardState(eraftpb::HardState &st)
{

}

// ApplySnapshot overwrites the contents of this Storage object with
// those of the given snapshot.
bool PeerStorage::ApplySnapshot(eraftpb::Snapshot &snap)
{

}

// CreateSnapshot makes a snapshot which can be retrieved with Snapshot() and
// can be used to reconstruct the state at that point.
// If any configuration changes have been made since the last compaction,
// the result of the last ApplyConfChange must be passed in.
eraftpb::Snapshot PeerStorage::CreateSnapshot(uint64_t i, eraftpb::ConfState* cs, const char* bytes)
{

}

// Compact discards all log entries prior to compactIndex.
// It is the application's responsibility to not attempt to compact an index
// greater than raftLog.applied.
bool PeerStorage::Compact(uint64_t compactIndex)
{

}

// Append the new entries to storage.
// entries[0].Index > ms.entries[0].Index
bool PeerStorage::Append(std::vector<eraftpb::Entry> entries)
{
    
}

} // namespace kvserver