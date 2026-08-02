#pragma once
// grpc_server.h — gRPC service implementation
// Corresponds to Go: grpc_server.go

#include <grpcpp/grpcpp.h>
#include "raft.grpc.pb.h"

#include "raft/raft.h"

namespace raft {

class RaftgRPCServiceImpl final : public raftpb::RaftService::Service {
public:
    explicit RaftgRPCServiceImpl(std::shared_ptr<Raft> rf) : rf_(std::move(rf)) {}

    grpc::Status RequestVote(grpc::ServerContext* context,
                             const raftpb::RequestVoteRequest* req,
                             raftpb::RequestVoteResponse* resp) override;

    grpc::Status AppendEntries(grpc::ServerContext* context,
                               const raftpb::AppendEntriesRequest* req,
                               raftpb::AppendEntriesResponse* resp) override;

    grpc::Status InstallSnapshot(grpc::ServerContext* context,
                                 const raftpb::InstallSnapshotRequest* req,
                                 raftpb::InstallSnapshotResponse* resp) override;

private:
    std::shared_ptr<Raft> rf_;
};

} // namespace raft
