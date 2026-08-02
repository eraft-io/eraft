// clerk.cpp — Clerk client implementation
// Corresponds to Go: kvraft/client.go

#include "raft/clerk.h"
#include "raft/kvserver.h"

#include <chrono>
#include <random>
#include <thread>

namespace raft {

// ── nrand — generate random int64 ────────────────────────────
static int64_t nrand() {
    static thread_local std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<int64_t> dist(0, (int64_t(1) << 62) - 1);
    return dist(rng);
}

// ── Clerk constructor ────────────────────────────────────────

Clerk::Clerk(std::vector<std::shared_ptr<KVPeer>> servers)
    : servers_(std::move(servers)),
      leaderId_(0),
      clientId_(nrand()),
      commandId_(0)
{}

// ── Public API ───────────────────────────────────────────────

std::string Clerk::Get(const std::string& key) {
    return doCommand(key, "", OpGet);
}

void Clerk::Put(const std::string& key, const std::string& value) {
    doCommand(key, value, OpPut);
}

void Clerk::Append(const std::string& key, const std::string& value) {
    doCommand(key, value, OpAppend);
}

// ── doCommand — core retry loop ──────────────────────────────

std::string Clerk::doCommand(const std::string& key, const std::string& value, OperationOp op) {
    CommandRequest req;
    req.key       = key;
    req.value     = value;
    req.op        = op;
    req.clientId  = clientId_;
    req.commandId = commandId_;

    for (;;) {
        CommandResponse resp;
        bool ok = servers_[leaderId_]->Command(req, resp);

        if (!ok || resp.err == ErrWrongLeader || resp.err == ErrTimeout) {
            leaderId_ = (leaderId_ + 1) % static_cast<int>(servers_.size());
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        commandId_++;
        return resp.value;
    }
}

// ── InMemKVPeer ──────────────────────────────────────────────

bool InMemKVPeer::Command(const CommandRequest& req, CommandResponse& resp) {
    auto t = target_.lock();
    if (!t || !enabled_) return false;
    t->HandleCommand(req, resp);
    return true;
}

} // namespace raft
