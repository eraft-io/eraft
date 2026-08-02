// grpc_server.cpp — gRPC service implementation
// Corresponds to Go: grpc_server.go

#include "raft/grpc_server.h"

namespace raft {

grpc::Status RaftgRPCServiceImpl::RequestVote(
    grpc::ServerContext* /*context*/,
    const raftpb::RequestVoteRequest* req,
    raftpb::RequestVoteResponse* resp)
{
    RequestVoteRequest args;
    args.term          = static_cast<int>(req->term());
    args.candidate_id  = static_cast<int>(req->candidate_id());
    args.last_log_index = static_cast<int>(req->last_log_index());
    args.last_log_term  = static_cast<int>(req->last_log_term());

    RequestVoteResponse reply;
    rf_->HandleRequestVote(args, reply);

    resp->set_term(reply.term);
    resp->set_vote_granted(reply.vote_granted);
    return grpc::Status::OK;
}

grpc::Status RaftgRPCServiceImpl::AppendEntries(
    grpc::ServerContext* /*context*/,
    const raftpb::AppendEntriesRequest* req,
    raftpb::AppendEntriesResponse* resp)
{
    AppendEntriesRequest args;
    args.term          = static_cast<int>(req->term());
    args.leader_id     = static_cast<int>(req->leader_id());
    args.prev_log_index = static_cast<int>(req->prev_log_index());
    args.prev_log_term  = static_cast<int>(req->prev_log_term());
    args.leader_commit  = static_cast<int>(req->leader_commit());

    for (const auto& pe : req->entries()) {
        Entry e;
        e.index = static_cast<int>(pe.index());
        e.term  = static_cast<int>(pe.term());
        if (!pe.command().empty()) {
            e.command.assign(pe.command().begin(), pe.command().end());
        }
        args.entries.push_back(std::move(e));
    }

    AppendEntriesResponse reply;
    rf_->HandleAppendEntries(args, reply);

    resp->set_term(reply.term);
    resp->set_success(reply.success);
    resp->set_conflict_index(reply.conflict_index);
    resp->set_conflict_term(reply.conflict_term);
    return grpc::Status::OK;
}

grpc::Status RaftgRPCServiceImpl::InstallSnapshot(
    grpc::ServerContext* /*context*/,
    const raftpb::InstallSnapshotRequest* req,
    raftpb::InstallSnapshotResponse* resp)
{
    InstallSnapshotRequest args;
    args.term               = static_cast<int>(req->term());
    args.leader_id          = static_cast<int>(req->leader_id());
    args.last_included_index = static_cast<int>(req->last_included_index());
    args.last_included_term  = static_cast<int>(req->last_included_term());
    if (!req->data().empty()) {
        args.data.assign(req->data().begin(), req->data().end());
    }

    InstallSnapshotResponse reply;
    rf_->HandleInstallSnapshot(args, reply);

    resp->set_term(reply.term);
    return grpc::Status::OK;
}

} // namespace raft
