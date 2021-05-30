#ifndef ERAFT_RAFTCORE_LOG_H_
#define ERAFT_RAFTCORE_LOG_H_

#include <RaftCore/MemoryStorage.h>

// RaftLog manage the log entries, its struct look like:
//
//  snapshot/first.....applied....committed....stabled.....last
//  --------|------------------------------------------------|
//                            log entries
//
// for simplify the RaftLog implement should manage all log entries
// that not truncated

namespace eraft
{

enum class NodeState {
    StateFollower,
    StateCandidate,
    StateLeader,
};

class RaftLog {

public:
    friend class RaftContext;

    friend class RawNode;

    RaftLog(StorageInterface &st);

    ~RaftLog();

    uint64_t LastIndex();

    uint64_t Term(uint64_t i);

private:

    void MaybeCompact();

    std::vector<eraftpb::Entry> UnstableEntries();

    std::vector<eraftpb::Entry> NextEnts();

    uint64_t ToSliceIndex(uint64_t i);

    uint64_t ToEntryIndex(uint64_t i);

	// storage contains all stable entries since the last snapshot.
    StorageInterface *storage_; // point to real storage

	// committed is the highest log position that is known to be in
	// stable storage on a quorum of nodes.
    uint64_t commited_;

	// applied is the highest log position that the application has
	// been instructed to apply to its state machine.
	// Invariant: applied <= committed
    uint64_t applied_;

	// log entries with index <= stabled are persisted to storage.
	// It is used to record the logs that are not persisted by storage yet.
	// Everytime handling `Ready`, the unstabled logs will be included.
    uint64_t stabled_;

	// all entries that have not yet compact.
    std::vector<eraftpb::Entry> entries_;

	// the incoming unstable snapshot, if any.
    eraftpb::Snapshot* pendingSnapshot_;

    uint64_t firstIndex_;

};
    
} // namespace eraft


#endif // ERAFT_RAFTCORE_LOG_H_