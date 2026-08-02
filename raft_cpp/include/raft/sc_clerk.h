#pragma once
// sc_clerk.h — Clerk client for ShardCtrler
// Corresponds to Go: shardctrler/client.go

#include "raft/sc_common.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace raft {

// ── SCClerkPeer interface ────────────────────────────────────
// Abstract interface for Clerk to communicate with ShardCtrler.
// In tests, InMemSCClerkPeer directly calls ShardCtrler::HandleCommand().
class SCClerkPeer {
public:
    virtual ~SCClerkPeer() = default;
    // Returns true if the call succeeded, false if network error.
    virtual bool Command(const SCCommandRequest& req, SCCommandResponse& resp) = 0;
};

// ── SCClerk ──────────────────────────────────────────────────
class SCClerk {
public:
    explicit SCClerk(std::vector<std::shared_ptr<SCClerkPeer>> servers);

    SCConfig Query(int num);
    void Join(const std::map<int, std::vector<std::string>>& servers);
    void Leave(const std::vector<int>& gids);
    void Move(int shard, int gid);

    int64_t ClientId() const { return clientId_; }

private:
    SCConfig doCommand(const SCCommandRequest& req);

    std::vector<std::shared_ptr<SCClerkPeer>> servers_;
    int     leaderId_  = 0;
    int64_t clientId_  = 0;
    int64_t commandId_ = 0;
};

// ── InMemSCClerkPeer ─────────────────────────────────────────
// A SCClerkPeer that directly calls ShardCtrler::HandleCommand() for testing.
class ShardCtrler;  // forward declaration

class InMemSCClerkPeer : public SCClerkPeer {
public:
    InMemSCClerkPeer() = default;
    void setTarget(std::shared_ptr<ShardCtrler> target) { target_ = target; }
    bool enabled() const { return enabled_; }
    void setEnabled(bool e) { enabled_ = e; }

    bool Command(const SCCommandRequest& req, SCCommandResponse& resp) override;

private:
    std::weak_ptr<ShardCtrler> target_;
    bool enabled_ = false;
};

} // namespace raft
