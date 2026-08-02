#pragma once
// config.h — Test configuration / framework
// Corresponds to Go: config.go
//
// Provides an in-memory network simulation for testing Raft
// without actual network I/O. RaftPeers call each other
// directly through shared pointers with enable/disable control.

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "raft/raft.h"
#include "raft/raft_peer.h"
#include "raft/types.h"
#include "raft/util.h"
#include "raft/persister.h"

namespace raft {

// ── InMemPeer ────────────────────────────────────────────────
// A RaftPeer that forwards RPC calls to a target Raft instance
// via shared pointer, with an enable/disable flag.
class InMemPeer : public RaftPeer {
public:
    // target is set later via setTarget(); we hold a weak_ptr
    // so we don't prevent cleanup.
    void setTarget(std::shared_ptr<Raft> target) { target_ = target; }

    bool enabled() const { return enabled_.load(); }
    void setEnabled(bool e) { enabled_.store(e); }

    bool RequestVote(const RequestVoteRequest& args,
                     RequestVoteResponse& reply) override {
        auto t = target_.lock();
        if (!t || !enabled_.load()) return false;
        t->HandleRequestVote(args, reply);
        return true;
    }

    bool AppendEntries(const AppendEntriesRequest& args,
                       AppendEntriesResponse& reply) override {
        auto t = target_.lock();
        if (!t || !enabled_.load()) return false;
        t->HandleAppendEntries(args, reply);
        return true;
    }

    bool InstallSnapshot(const InstallSnapshotRequest& args,
                         InstallSnapshotResponse& reply) override {
        auto t = target_.lock();
        if (!t || !enabled_.load()) return false;
        t->HandleInstallSnapshot(args, reply);
        return true;
    }

private:
    std::weak_ptr<Raft> target_;
    std::atomic<bool> enabled_{false};
};

// ── Config ───────────────────────────────────────────────────
class Config {
public:
    Config(int n, bool unreliable, bool snapshot);
    ~Config();

    // ── Public test helpers ──────────────────────────────────
    int  checkOneLeader();
    int  checkTerms();
    void checkNoLeader();

    // How many servers have committed index?
    std::pair<int, std::vector<uint8_t>> nCommitted(int index);

    // Wait for at least n servers to commit.
    std::vector<uint8_t> wait(int index, int n, int startTerm);

    // Drive full agreement for one command.
    int one(const std::vector<uint8_t>& cmd, int expectedServers, bool retry);

    void connect(int i);
    void disconnect(int i);

    // Crash server i (save persistent state).
    void crash1(int i);
    // Start (or restart) server i.
    void start1(int i);

    void cleanup();

    int  rpcCount(int server) const { (void)server; return 0; }
    int  rpcTotal() const { return 0; }
    long bytesTotal() const { return 0; }
    int  LogSize() const;

    void setUnreliable(bool u) { unreliable_ = u; }
    void setLongReordering(bool) {} // no-op for now

    void begin(const std::string& description);
    void end();

    // Access for tests
    std::shared_ptr<Raft>& rafts(int i) { return rafts_[i]; }
    bool isConnected(int i) const { return connected_[i]; }

    int n() const { return n_; }

private:
    std::string checkLogs(int i, const ApplyMsg& m);

    static void applyThreadStatic(Config* cfg, int serverIdx,
                                  std::shared_ptr<BlockingQueue<ApplyMsg>> ch,
                                  bool doSnapshot);

    std::mutex mu_;
    int n_;
    bool unreliable_;
    bool snapshot_;

    std::vector<std::shared_ptr<Raft>> rafts_;
    std::vector<bool> connected_;
    std::vector<std::shared_ptr<Persister>> saved_;
    // peers_[i][j] = peer that server i uses to talk to server j
    std::vector<std::vector<std::shared_ptr<InMemPeer>>> peers_;

    std::vector<std::string> applyErr_;
    // committed logs per server: logs_[server][index] = command
    std::vector<std::map<int, std::vector<uint8_t>>> logs_;
    int maxIndex_ = 0;

    // Apply channel per server
    std::vector<std::shared_ptr<BlockingQueue<ApplyMsg>>> applyChs_;
    // Apply threads
    std::vector<std::thread> applyThreads_;

    std::chrono::steady_clock::time_point start_;
    // For begin/end stats
    std::chrono::steady_clock::time_point t0_;
    int cmds0_ = 0;
    int maxIndex0_ = 0;
};

} // namespace raft
