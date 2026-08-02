// grpc_client.cpp — gRPC client implementation
// Corresponds to Go: grpc_client.go

#include "raft/grpc_client.h"

#include <chrono>

namespace raft {

RaftgRPCClient::RaftgRPCClient(const std::string& addr)
    : channel_(grpc::CreateChannel(addr, grpc::InsecureChannelCredentials())),
      stub_(raftpb::RaftService::NewStub(channel_)) {}

RaftgRPCClient::~RaftgRPCClient() = default;

bool RaftgRPCClient::RequestVote(const RequestVoteRequest& args,
                                 RequestVoteResponse& reply) {
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() +
                     std::chrono::milliseconds(100));

    raftpb::RequestVoteRequest req;
    req.set_term(args.term);
    req.set_candidate_id(args.candidate_id);
    req.set_last_log_index(args.last_log_index);
    req.set_last_log_term(args.last_log_term);

    raftpb::RequestVoteResponse resp;
    auto status = stub_->RequestVote(&ctx, req, &resp);
    if (!status.ok()) return false;

    reply.term        = static_cast<int>(resp.term());
    reply.vote_granted = resp.vote_granted();
    return true;
}

bool RaftgRPCClient::AppendEntries(const AppendEntriesRequest& args,
                                   AppendEntriesResponse& reply) {
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() +
                     std::chrono::milliseconds(100));

    raftpb::AppendEntriesRequest req;
    req.set_term(args.term);
    req.set_leader_id(args.leader_id);
    req.set_prev_log_index(args.prev_log_index);
    req.set_prev_log_term(args.prev_log_term);
    req.set_leader_commit(args.leader_commit);

    for (const auto& e : args.entries) {
        auto* pe = req.add_entries();
        pe->set_index(e.index);
        pe->set_term(e.term);
        if (!e.command.empty()) {
            pe->set_command(e.command.data(), e.command.size());
        }
    }

    raftpb::AppendEntriesResponse resp;
    auto status = stub_->AppendEntries(&ctx, req, &resp);
    if (!status.ok()) return false;

    reply.term          = static_cast<int>(resp.term());
    reply.success       = resp.success();
    reply.conflict_index = static_cast<int>(resp.conflict_index());
    reply.conflict_term  = static_cast<int>(resp.conflict_term());
    return true;
}

bool RaftgRPCClient::InstallSnapshot(const InstallSnapshotRequest& args,
                                     InstallSnapshotResponse& reply) {
    grpc::ClientContext ctx;
    ctx.set_deadline(std::chrono::system_clock::now() +
                     std::chrono::seconds(1));

    raftpb::InstallSnapshotRequest req;
    req.set_term(args.term);
    req.set_leader_id(args.leader_id);
    req.set_last_included_index(args.last_included_index);
    req.set_last_included_term(args.last_included_term);
    if (!args.data.empty()) {
        req.set_data(args.data.data(), args.data.size());
    }

    raftpb::InstallSnapshotResponse resp;
    auto status = stub_->InstallSnapshot(&ctx, req, &resp);
    if (!status.ok()) return false;

    reply.term = static_cast<int>(resp.term());
    return true;
}

} // namespace raft
