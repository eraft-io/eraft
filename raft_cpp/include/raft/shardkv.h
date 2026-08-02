#pragma once
// shardkv.h — ShardKV class definition
// Corresponds to Go: shardkv/server.go

#include "raft/skv_common.h"
#include "raft/sc_clerk.h"
#include "raft/raft.h"
#include "raft/types.h"
#include "raft/util.h"
#include "raft/persister.h"
#include "raft/raft_peer.h"

#include <array>
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace raft {

// ── ShardKVPeerProvider ──────────────────────────────────────
// Interface for cross-group communication (migration/GC).
// In tests, InMemShardKVPeerProvider directly calls target ShardKV methods.
class ShardKV;  // forward declaration

struct ShardKVPeerHandle {
    virtual ~ShardKVPeerHandle() = default;
    virtual bool GetShardsData(const ShardOperationRequest& req,
                               ShardOperationResponse& resp) = 0;
    virtual bool DeleteShardsData(const ShardOperationRequest& req,
                                  ShardOperationResponse& resp) = 0;
};

using ShardKVPeerProvider = std::function<
    std::shared_ptr<ShardKVPeerHandle>(int gid, int serverIdx, const std::string& serverAddr)>;

// ── ShardKV ──────────────────────────────────────────────────
class ShardKV : public std::enable_shared_from_this<ShardKV> {
public:
    // Factory — returns a started ShardKV
    static std::shared_ptr<ShardKV> Make(
        std::vector<std::shared_ptr<RaftPeer>> peers,
        int me,
        std::shared_ptr<Persister> persister,
        int maxRaftState,
        int gid,
        std::shared_ptr<SCClerk> sc,
        ShardKVPeerProvider peerProvider);

    ~ShardKV();

    // KV command entry point
    void Command(const SKVCommandRequest& req, SKVCommandResponse& resp);

    // Cross-group RPC for shard migration
    void GetShardsData(const ShardOperationRequest& req, ShardOperationResponse& resp);
    void DeleteShardsData(const ShardOperationRequest& req, ShardOperationResponse& resp);

    void Kill();
    bool killed() const { return dead_.load(std::memory_order_relaxed); }

    std::shared_ptr<raft::Raft> getRaft() const { return rf_; }
    int getGid() const { return gid_; }

    // Diagnostics: returns current config num and count of non-Serving shards
    struct DebugInfo {
        int configNum;
        int nonServingCount;
        std::array<ShardStatus, NShards> shardStatuses;
    };
    DebugInfo GetDebugInfo();

private:
    ShardKV() = default;

    // ── Members ──────────────────────────────────────────────
    mutable std::mutex mu_;
    std::atomic<bool> dead_{false};

    std::shared_ptr<Raft> rf_;
    std::shared_ptr<BlockingQueue<ApplyMsg>> applyCh_;
    int maxRaftState_ = -1;
    int lastApplied_  = 0;

    int gid_;  // group ID
    std::shared_ptr<SCClerk> sc_;  // shardctrler clerk

    // Shard storage — in-memory per-shard maps
    std::array<std::map<std::string, std::string>, NShards> shards_;
    std::array<ShardStatus, NShards> shardStatus_ = {};

    // Configuration
    SCConfig currentConfig_;
    SCConfig lastConfig_;

    // Duplicate detection
    std::unordered_map<int64_t, SKVOperationContext> lastOperations_;

    // Notify channels for leader's Command() waiting
    std::unordered_map<int, std::shared_ptr<BlockingQueue<SKVCommandResponse>>> notifyChans_;

    // Cross-group communication
    ShardKVPeerProvider peerProvider_;

    // Threads
    std::thread applierThread_;
    std::thread configureThread_;
    std::thread migrationThread_;
    std::thread gcThread_;
    std::thread emptyEntryThread_;

    // ── Private methods ──────────────────────────────────────
    void applier();

    // Apply handlers for each command type
    SKVCommandResponse applyOperation(const SKVCommandRequest& req);
    SKVCommandResponse applyConfiguration(const SCConfig& nextConfig);
    SKVCommandResponse applyInsertShards(const ShardOperationResponse& info);
    SKVCommandResponse applyDeleteShards(const ShardOperationRequest& req);

    // Monitor actions (run in dedicated threads)
    void configureAction();
    void migrationAction();
    void gcAction();
    void checkEntryInCurrentTermAction();
    void monitorLoop(std::function<void()> action, std::chrono::milliseconds timeout);

    // Helpers
    bool canServe(int shardID);
    bool isDuplicateRequest(int64_t clientId, int64_t requestId);
    void updateShardStatus(const SCConfig& nextConfig);
    std::map<int, std::vector<int>> getShardIDsByStatus(ShardStatus status);
    void clearShardData(int shardID);

    // Snapshot
    bool needSnapshot();
    void takeSnapshot(int index);
    void restoreSnapshot(const std::vector<uint8_t>& snap);
    void initStateMachines();

    // Notify channels
    std::shared_ptr<BlockingQueue<SKVCommandResponse>> getNotifyChan(int index);
    void removeOutdatedNotifyChan(int index);

    // Execute command through Raft
    void Execute(const SKVCommand& cmd, SKVCommandResponse& resp);
};

} // namespace raft
