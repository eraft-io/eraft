#pragma once
// raft_peer.h — abstract interface for Raft peer communication
// Corresponds to Go: RaftPeer interface in raft.go

#include "raft/types.h"

namespace raft {

class RaftPeer {
public:
    virtual ~RaftPeer() = default;

    virtual bool RequestVote(const RequestVoteRequest& args,
                             RequestVoteResponse& reply) = 0;

    virtual bool AppendEntries(const AppendEntriesRequest& args,
                               AppendEntriesResponse& reply) = 0;

    virtual bool InstallSnapshot(const InstallSnapshotRequest& args,
                                 InstallSnapshotResponse& reply) = 0;
};

} // namespace raft
