#pragma once
// shardctrler.h — ShardCtrler class definition
// Corresponds to Go: shardctrler/server.go

#include "raft/sc_common.h"
#include "raft/sc_statemachine.h"
#include "raft/raft.h"
#include "raft/types.h"
#include "raft/util.h"
#include "raft/persister.h"
#include "raft/raft_peer.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace raft {

class ShardCtrler : public std::enable_shared_from_this<ShardCtrler> {
public:
    // Factory — returns a started ShardCtrler
    static std::shared_ptr<ShardCtrler> Make(
        std::vector<std::shared_ptr<RaftPeer>> peers,
        int me,
        std::shared_ptr<Persister> persister);

    ~ShardCtrler();

    // RPC entry point — called by Clerk (directly or via gRPC)
    void HandleCommand(const SCCommandRequest& req, SCCommandResponse& resp);

    void Kill();
    bool killed() const { return dead_.load(std::memory_order_relaxed); }

    std::shared_ptr<raft::Raft> getRaft() const { return rf_; }

    struct Status {
        int id;
        std::string state;
        int term;
        int lastApplied;
        int commitIndex;
        int64_t dataSize;
    };
    Status GetStatus();

private:
    ShardCtrler() = default;

    // ── Members ────────────────────────────────────────────
    mutable std::mutex mu_;
    std::atomic<bool> dead_{false};

    std::shared_ptr<Raft> rf_;
    std::shared_ptr<BlockingQueue<ApplyMsg>> applyCh_;
    int lastApplied_ = 0;

    std::unique_ptr<ConfigStateMachine> stateMachine_;
    std::unordered_map<int64_t, SCOperationContext> lastOperations_;

    // notifyChans_[index] = channel to notify leader's Command() goroutine
    std::unordered_map<int, std::shared_ptr<BlockingQueue<SCCommandResponse>>> notifyChans_;

    std::thread applierThread_;

    // ── Private methods ────────────────────────────────────
    void applier();
    bool isDuplicateRequest(int64_t clientId, int64_t requestId);
    SCCommandResponse applyLogToStateMachine(const SCCommand& cmd);
    std::shared_ptr<BlockingQueue<SCCommandResponse>> getNotifyChan(int index);
    void removeOutdatedNotifyChan(int index);
};

} // namespace raft
