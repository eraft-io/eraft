#pragma once
// kvserver.h — KVServer class definition
// Corresponds to Go: kvraft/server.go (KVServer)

#include "raft/kvcommon.h"
#include "raft/kvstatemachine.h"
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

class KVServer : public std::enable_shared_from_this<KVServer> {
public:
    // Factory — returns a started KVServer
    static std::shared_ptr<KVServer> Make(
        std::vector<std::shared_ptr<RaftPeer>> peers,
        int me,
        std::shared_ptr<Persister> persister,
        int maxRaftState,
        const std::string& dbPath);

    ~KVServer();

    // RPC entry point — called by Clerk (directly or via gRPC)
    void HandleCommand(const CommandRequest& req, CommandResponse& resp);

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
    KVServer() = default;

    // ── Members ────────────────────────────────────────────
    mutable std::mutex mu_;
    std::atomic<bool> dead_{false};

    std::shared_ptr<Raft> rf_;
    std::shared_ptr<BlockingQueue<ApplyMsg>> applyCh_;
    int maxRaftState_ = -1;
    int lastApplied_  = 0;

    std::unique_ptr<KVStateMachine> stateMachine_;
    std::unordered_map<int64_t, OperationContext> lastOperations_;

    // notifyChans_[index] = channel to notify leader's Command() goroutine
    std::unordered_map<int, std::shared_ptr<BlockingQueue<CommandResponse>>> notifyChans_;

    std::thread applierThread_;

    // ── Private methods ────────────────────────────────────
    void applier();
    void applyOneMessage(const ApplyMsg& message, int& lastIndex);
    bool needSnapshot();
    void takeSnapshot(int index);
    void restoreSnapshot(const std::vector<uint8_t>& snap);
    bool isDuplicateRequest(int64_t clientId, int64_t requestId);
    CommandResponse applyLogToStateMachine(const struct Command& cmd);
    std::shared_ptr<BlockingQueue<CommandResponse>> getNotifyChan(int index);
    void removeOutdatedNotifyChan(int index);
};

} // namespace raft
