// skv_clerk.cpp — ShardKVClerk client implementation
// Corresponds to Go: shardkv/client.go

#include "raft/skv_clerk.h"
#include "raft/shardkv.h"

#include <chrono>
#include <random>
#include <thread>

namespace raft {

// ── nrand — generate random int64 ────────────────────────────
static int64_t skv_nrand() {
    static thread_local std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<int64_t> dist(0, (int64_t(1) << 62) - 1);
    return dist(rng);
}

// ── ShardKVClerk constructor ─────────────────────────────────

ShardKVClerk::ShardKVClerk(std::shared_ptr<SCClerk> sm)
    : sm_(std::move(sm)),
      clientId_(skv_nrand()),
      commandId_(0)
{
    config_ = sm_->Query(-1);
}

// ── Public API ───────────────────────────────────────────────

std::string ShardKVClerk::Get(const std::string& key) {
    SKVCommandRequest req;
    req.key = key;
    req.op  = OpGet;
    return doCommand(req);
}

void ShardKVClerk::Put(const std::string& key, const std::string& value) {
    SKVCommandRequest req;
    req.key   = key;
    req.value = value;
    req.op    = OpPut;
    doCommand(req);
}

void ShardKVClerk::Append(const std::string& key, const std::string& value) {
    SKVCommandRequest req;
    req.key   = key;
    req.value = value;
    req.op    = OpAppend;
    doCommand(req);
}

// ── SetPeers ─────────────────────────────────────────────────

void ShardKVClerk::SetPeers(int gid, std::vector<std::shared_ptr<ShardKVClientPeer>> peers) {
    clients_[gid] = std::move(peers);
}

// ── doCommand — core retry loop ──────────────────────────────

std::string ShardKVClerk::doCommand(const SKVCommandRequest& baseReq) {
    SKVCommandRequest req = baseReq;
    req.clientId  = clientId_;
    req.commandId = commandId_;

    // Retry until success (like Go version).
    // Use a long timeout as safety net (Go version has no timeout).
    auto startTime = std::chrono::steady_clock::now();
    const int kClerkTimeoutMs = 120000;  // 120 seconds safety net

    while (true) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed > kClerkTimeoutMs) {
            return "";
        }

        int shard = key2shard(req.key);
        int gid = config_.Shards[shard];

        auto it = config_.Groups.find(gid);
        if (it != config_.Groups.end()) {
            auto& servers = it->second;

            if (leaderIds_.find(gid) == leaderIds_.end()) {
                leaderIds_[gid] = 0;
            }

            // Check if we have peers for this gid
            auto peerIt = clients_.find(gid);
            if (peerIt == clients_.end() || peerIt->second.empty()) {
                // No peers available, refresh config and retry
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                config_ = sm_->Query(-1);
                continue;
            }

            auto& peers = peerIt->second;
            int oldLeaderId = leaderIds_[gid];
            int newLeaderId = oldLeaderId;

            for (size_t attempt = 0; attempt < peers.size(); ++attempt) {
                SKVCommandResponse resp;
                bool ok = peers[newLeaderId]->Command(req, resp);

                if (ok && (resp.err == SKV_OK || resp.err == SKV_ErrNoKey)) {
                    leaderIds_[gid] = newLeaderId;
                    commandId_++;
                    return resp.value;
                } else if (ok && resp.err == SKV_ErrWrongGroup) {
                    // Config changed, break out to refresh
                    break;
                } else {
                    newLeaderId = (newLeaderId + 1) % static_cast<int>(peers.size());
                    if (newLeaderId == oldLeaderId) break;
                    continue;
                }
            }
        }

        // Refresh configuration and retry
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        config_ = sm_->Query(-1);
    }
}

// ── InMemShardKVClientPeer ───────────────────────────────────

bool InMemShardKVClientPeer::Command(const SKVCommandRequest& req, SKVCommandResponse& resp) {
    auto t = target_.lock();
    if (!t || !enabled_) return false;
    t->Command(req, resp);
    return true;
}

} // namespace raft
