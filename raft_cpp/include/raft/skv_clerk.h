#pragma once
// skv_clerk.h — ShardKV Clerk client
// Corresponds to Go: shardkv/client.go

#include "raft/skv_common.h"
#include "raft/sc_clerk.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace raft {

// ── ShardKVClientPeer ────────────────────────────────────────
// Abstract interface for Clerk to communicate with a ShardKV server.
class ShardKVClientPeer {
public:
    virtual ~ShardKVClientPeer() = default;
    virtual bool Command(const SKVCommandRequest& req, SKVCommandResponse& resp) = 0;
};

// ── ShardKVClerk ─────────────────────────────────────────────
class ShardKVClerk {
public:
    explicit ShardKVClerk(std::shared_ptr<SCClerk> sm);

    std::string Get(const std::string& key);
    void Put(const std::string& key, const std::string& value);
    void Append(const std::string& key, const std::string& value);

    int64_t ClientId() const { return clientId_; }

    // Register peers for a given gid (set by test config)
    void SetPeers(int gid, std::vector<std::shared_ptr<ShardKVClientPeer>> peers);

private:
    std::string doCommand(const SKVCommandRequest& req);

    std::shared_ptr<SCClerk> sm_;
    SCConfig config_;
    // gid -> peers
    std::map<int, std::vector<std::shared_ptr<ShardKVClientPeer>>> clients_;
    // gid -> leader index
    std::map<int, int> leaderIds_;
    int64_t clientId_  = 0;
    int64_t commandId_ = 0;
};

// ── InMemShardKVClientPeer ───────────────────────────────────
// Directly calls ShardKV::Command() for testing.
class ShardKV;  // forward declaration

class InMemShardKVClientPeer : public ShardKVClientPeer {
public:
    InMemShardKVClientPeer() = default;
    void setTarget(std::shared_ptr<ShardKV> target) { target_ = target; }
    bool enabled() const { return enabled_; }
    void setEnabled(bool e) { enabled_ = e; }

    bool Command(const SKVCommandRequest& req, SKVCommandResponse& resp) override;

private:
    std::weak_ptr<ShardKV> target_;
    bool enabled_ = false;
};

} // namespace raft
