// sc_clerk.cpp — SCClerk client implementation
// Corresponds to Go: shardctrler/client.go

#include "raft/sc_clerk.h"
#include "raft/shardctrler.h"

#include <chrono>
#include <random>
#include <thread>

namespace raft {

// ── nrand — generate random int64 ────────────────────────────
static int64_t sc_nrand() {
    static thread_local std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<int64_t> dist(0, (int64_t(1) << 62) - 1);
    return dist(rng);
}

// ── SCClerk constructor ──────────────────────────────────────

SCClerk::SCClerk(std::vector<std::shared_ptr<SCClerkPeer>> servers)
    : servers_(std::move(servers)),
      leaderId_(0),
      clientId_(sc_nrand()),
      commandId_(0)
{}

// ── Public API ───────────────────────────────────────────────

SCConfig SCClerk::Query(int num) {
    SCCommandRequest req;
    req.Num = num;
    req.Op = SCOpQuery;
    return doCommand(req);
}

void SCClerk::Join(const std::map<int, std::vector<std::string>>& servers) {
    SCCommandRequest req;
    req.Servers = servers;
    req.Op = SCOpJoin;
    doCommand(req);
}

void SCClerk::Leave(const std::vector<int>& gids) {
    SCCommandRequest req;
    req.GIDs = gids;
    req.Op = SCOpLeave;
    doCommand(req);
}

void SCClerk::Move(int shard, int gid) {
    SCCommandRequest req;
    req.Shard = shard;
    req.GID = gid;
    req.Op = SCOpMove;
    doCommand(req);
}

// ── doCommand — core retry loop ──────────────────────────────

SCConfig SCClerk::doCommand(const SCCommandRequest& req) {
    SCCommandRequest r = req;
    r.ClientId = clientId_;
    r.CommandId = commandId_;

    for (;;) {
        SCCommandResponse resp;
        bool ok = servers_[leaderId_]->Command(r, resp);

        if (!ok || resp.Err == SC_ErrWrongLeader || resp.Err == SC_ErrTimeout) {
            leaderId_ = (leaderId_ + 1) % static_cast<int>(servers_.size());
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        commandId_++;
        return resp.Config;
    }
}

// ── InMemSCClerkPeer ─────────────────────────────────────────

bool InMemSCClerkPeer::Command(const SCCommandRequest& req, SCCommandResponse& resp) {
    auto t = target_.lock();
    if (!t || !enabled_) return false;
    t->HandleCommand(req, resp);
    return true;
}

} // namespace raft
