#pragma once
// raft.h — Core Raft peer implementation
// Corresponds to Go: raft.go

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "raft/persister.h"
#include "raft/raft_peer.h"
#include "raft/types.h"
#include "raft/util.h"

namespace raft {

class Raft : public std::enable_shared_from_this<Raft> {
public:
    ~Raft();
    // Public API — thread-safe
    // Returns (currentTerm, isLeader).
    std::pair<int, bool> GetState();

    // Returns (me, stateStr, currentTerm, lastApplied, commitIndex).
    struct Status { int me; std::string state; int term; int lastApplied; int commitIndex; };
    Status GetStatus();

    int GetRaftStateSize();

    // Install a snapshot from the service layer (only if still valid).
    bool CondInstallSnapshot(int lastIncludedTerm, int lastIncludedIndex,
                             const std::vector<uint8_t>& snapshot);

    // Service layer created a snapshot up to and including index.
    void Snapshot(int index, const std::vector<uint8_t>& snapshot);

    // RPC handlers (called by server layer)
    void HandleRequestVote(const RequestVoteRequest& req, RequestVoteResponse& resp);
    void HandleAppendEntries(const AppendEntriesRequest& req, AppendEntriesResponse& resp);
    void HandleInstallSnapshot(const InstallSnapshotRequest& req, InstallSnapshotResponse& resp);

    // Start agreement on a new log entry.
    // Returns (index, term, isLeader).
    struct StartResult { int index; int term; bool isLeader; };
    StartResult Start(const std::vector<uint8_t>& command);

    bool HasLogInCurrentTerm();

    void Kill();
    bool killed() const { return dead_.load(std::memory_order_relaxed); }
    int  Me() const { return me_; }

    // Factory
    static std::shared_ptr<Raft> Make(
        std::vector<std::shared_ptr<RaftPeer>> peers,
        int me,
        std::shared_ptr<Persister> persister,
        std::shared_ptr<BlockingQueue<ApplyMsg>> applyCh);

private:
    Raft() = default;

    // ── Members ──────────────────────────────────────────────
    mutable std::mutex mu_;  // protects all shared state below

    std::vector<std::shared_ptr<RaftPeer>> peers_;
    std::shared_ptr<Persister> persister_;
    int  me_  = 0;
    std::atomic<bool> dead_{false};

    std::shared_ptr<BlockingQueue<ApplyMsg>> applyCh_;
    std::condition_variable_any applyCond_;       // wakes applier when commitIndex advances

    // Per-peer replicator condition: signals replicator thread to send entries
    std::vector<std::unique_ptr<std::mutex>> replicatorMu_;
    std::vector<std::unique_ptr<std::condition_variable>> replicatorCv_;

    NodeState state_ = NodeState::Follower;

    // Persistent state
    int currentTerm_ = 0;
    int votedFor_    = -1;
    std::vector<Entry> logs_;   // logs_[0] is the dummy entry

    // Volatile state
    int commitIndex_ = 0;
    int lastApplied_ = 0;
    std::vector<int> nextIndex_;
    std::vector<int> matchIndex_;

    // Ticker-managed deadlines (protected by mu_)
    std::chrono::steady_clock::time_point electionDeadline_;
    std::chrono::steady_clock::time_point heartbeatDeadline_;

    // Background threads
    std::thread tickerThread_;
    std::thread applierThread_;
    std::vector<std::thread> replicatorThreads_;

    // ── Private helpers ──────────────────────────────────────
    void persist();
    std::vector<uint8_t> encodeState() const;
    void readPersist(const std::vector<uint8_t>& data);

    const Entry& getLastLog()  const { return logs_.back(); }
    const Entry& getFirstLog() const { return logs_.front(); }

    bool isLogUpToDate(int term, int index) const;
    bool matchLog(int term, int index) const;
    Entry appendNewEntry(const std::vector<uint8_t>& command);

    void changeState(NodeState newState);

    void advanceCommitIndexForLeader();
    void advanceCommitIndexForFollower(int leaderCommit);

    void StartElection();
    void BroadcastHeartbeat(bool isHeartbeat);
    bool replicateOneRound(int peer);

    void sendRequestVote(int server, const RequestVoteRequest& req, RequestVoteResponse& resp, bool& ok);
    void sendAppendEntries(int server, const AppendEntriesRequest& req, AppendEntriesResponse& resp, bool& ok);
    void sendInstallSnapshot(int server, const InstallSnapshotRequest& req, InstallSnapshotResponse& resp, bool& ok);

    RequestVoteRequest    genRequestVoteRequest() const;
    AppendEntriesRequest  genAppendEntriesRequest(int prevLogIndex) const;
    InstallSnapshotRequest genInstallSnapshotRequest() const;

    void handleAppendEntriesResponse(int peer, const AppendEntriesRequest& req,
                                     const AppendEntriesResponse& resp);
    void handleInstallSnapshotResponse(int peer, const InstallSnapshotRequest& req,
                                       const InstallSnapshotResponse& resp);

    bool needReplicating(int peer) const;

    // Background thread entry points
    void ticker();
    void applier();
    void replicator(int peer);
};

} // namespace raft
