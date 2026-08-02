#pragma once
// grpc_client.h — gRPC client implementing RaftPeer
// Corresponds to Go: grpc_client.go

#include <grpcpp/grpcpp.h>
#include "raft.grpc.pb.h"

#include "raft/raft_peer.h"

namespace raft {

class RaftgRPCClient : public RaftPeer {
public:
    explicit RaftgRPCClient(const std::string& addr);
    ~RaftgRPCClient() override;

    bool RequestVote(const RequestVoteRequest& args,
                     RequestVoteResponse& reply) override;

    bool AppendEntries(const AppendEntriesRequest& args,
                       AppendEntriesResponse& reply) override;

    bool InstallSnapshot(const InstallSnapshotRequest& args,
                         InstallSnapshotResponse& reply) override;

private:
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<raftpb::RaftService::Stub> stub_;
};

} // namespace raft
