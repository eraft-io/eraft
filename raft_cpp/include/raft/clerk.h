#pragma once
// clerk.h — Clerk client for KV Raft
// Corresponds to Go: kvraft/client.go

#include "raft/kvcommon.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace raft {

// ── KVPeer interface ─────────────────────────────────────────
// Abstract interface for Clerk to communicate with KVServer.
// In tests, InMemKVPeer directly calls KVServer::Command().
class KVPeer {
public:
    virtual ~KVPeer() = default;
    // Returns true if the call succeeded, false if network error.
    virtual bool Command(const CommandRequest& req, CommandResponse& resp) = 0;
};

// ── Clerk ────────────────────────────────────────────────────
class Clerk {
public:
    explicit Clerk(std::vector<std::shared_ptr<KVPeer>> servers);

    std::string Get(const std::string& key);
    void Put(const std::string& key, const std::string& value);
    void Append(const std::string& key, const std::string& value);

    int64_t ClientId() const { return clientId_; }

private:
    std::string doCommand(const std::string& key, const std::string& value, OperationOp op);

    std::vector<std::shared_ptr<KVPeer>> servers_;
    int     leaderId_  = 0;
    int64_t clientId_  = 0;
    int64_t commandId_ = 0;
};

// ── InMemKVPeer ──────────────────────────────────────────────
// A KVPeer that directly calls KVServer::Command() for testing.
class KVServer;  // forward declaration

class InMemKVPeer : public KVPeer {
public:
    InMemKVPeer() = default;
    void setTarget(std::shared_ptr<KVServer> target) { target_ = target; }
    bool enabled() const { return enabled_; }
    void setEnabled(bool e) { enabled_ = e; }

    bool Command(const CommandRequest& req, CommandResponse& resp) override;

private:
    std::weak_ptr<KVServer> target_;
    bool enabled_ = false;
};

} // namespace raft
