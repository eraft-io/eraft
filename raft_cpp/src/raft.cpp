// raft.cpp — Core Raft algorithm implementation
// Corresponds to Go: raft.go (726 lines)

#include "raft/raft.h"

#include <algorithm>
#include <cstring>

namespace raft {

// ── Simple binary serialization helpers ──────────────────────
namespace ser {

inline void writeInt(std::vector<uint8_t>& buf, int v) {
    const auto* p = reinterpret_cast<const uint8_t*>(&v);
    buf.insert(buf.end(), p, p + sizeof(v));
}

inline int readInt(const uint8_t*& p) {
    int v;
    std::memcpy(&v, p, sizeof(v));
    p += sizeof(v);
    return v;
}

inline void writeBytes(std::vector<uint8_t>& buf, const std::vector<uint8_t>& data) {
    int len = static_cast<int>(data.size());
    writeInt(buf, len);
    buf.insert(buf.end(), data.begin(), data.end());
}

inline std::vector<uint8_t> readBytes(const uint8_t*& p) {
    int len = readInt(p);
    std::vector<uint8_t> v(p, p + len);
    p += len;
    return v;
}

} // namespace ser

// ── Persistence ──────────────────────────────────────────────

void Raft::persist() {
    persister_->SaveRaftState(encodeState());
}

std::vector<uint8_t> Raft::encodeState() const {
    std::vector<uint8_t> buf;
    ser::writeInt(buf, currentTerm_);
    ser::writeInt(buf, votedFor_);
    ser::writeInt(buf, static_cast<int>(logs_.size()));
    for (const auto& e : logs_) {
        ser::writeInt(buf, e.index);
        ser::writeInt(buf, e.term);
        ser::writeBytes(buf, e.command);
    }
    return buf;
}

void Raft::readPersist(const std::vector<uint8_t>& data) {
    if (data.empty()) return;
    const uint8_t* p = data.data();
    currentTerm_ = ser::readInt(p);
    votedFor_    = ser::readInt(p);
    int n        = ser::readInt(p);
    logs_.clear();
    logs_.reserve(n);
    for (int i = 0; i < n; ++i) {
        Entry e;
        e.index   = ser::readInt(p);
        e.term    = ser::readInt(p);
        e.command = ser::readBytes(p);
        logs_.push_back(std::move(e));
    }
    // There will always be at least one entry (the dummy).
    lastApplied_  = logs_[0].index;
    commitIndex_  = logs_[0].index;
}

// ── Public API ───────────────────────────────────────────────

std::pair<int, bool> Raft::GetState() {
    std::lock_guard<std::mutex> lk(mu_);
    return {currentTerm_, state_ == NodeState::Leader};
}

Raft::Status Raft::GetStatus() {
    std::lock_guard<std::mutex> lk(mu_);
    return {me_, raft::to_string(state_), currentTerm_, lastApplied_, commitIndex_};
}

int Raft::GetRaftStateSize() {
    std::lock_guard<std::mutex> lk(mu_);
    return static_cast<int>(persister_->RaftStateSize());
}

bool Raft::HasLogInCurrentTerm() {
    std::lock_guard<std::mutex> lk(mu_);
    return getLastLog().term == currentTerm_;
}

// ── Snapshot ─────────────────────────────────────────────────

bool Raft::CondInstallSnapshot(int lastIncludedTerm, int lastIncludedIndex,
                               const std::vector<uint8_t>& snapshot) {
    std::lock_guard<std::mutex> lk(mu_);

    if (lastIncludedIndex <= commitIndex_) return false;

    if (lastIncludedIndex > getLastLog().index) {
        logs_.resize(1);
    } else {
        int firstIdx = getFirstLog().index;
        logs_ = std::vector<Entry>(logs_.begin() + (lastIncludedIndex - firstIdx),
                                   logs_.end());
        logs_[0].command.clear();
    }
    logs_[0].term  = lastIncludedTerm;
    logs_[0].index = lastIncludedIndex;
    lastApplied_ = lastIncludedIndex;
    commitIndex_ = lastIncludedIndex;

    persister_->SaveStateAndSnapshot(encodeState(), snapshot);
    return true;
}

void Raft::Snapshot(int index, const std::vector<uint8_t>& snapshot) {
    std::lock_guard<std::mutex> lk(mu_);
    int snapshotIndex = getFirstLog().index;
    if (index <= snapshotIndex) return;

    logs_ = std::vector<Entry>(logs_.begin() + (index - snapshotIndex),
                               logs_.end());
    logs_[0].command.clear();
    persister_->SaveStateAndSnapshot(encodeState(), snapshot);
}

// ── RPC Handlers ─────────────────────────────────────────────

void Raft::HandleRequestVote(const RequestVoteRequest& req, RequestVoteResponse& resp) {
    std::lock_guard<std::mutex> lk(mu_);

    auto deferPersist = [&]{ persist(); };
    struct Guard { std::function<void()> fn; ~Guard(){ fn(); } };
    Guard g{deferPersist};

    if (req.term < currentTerm_ ||
        (req.term == currentTerm_ && votedFor_ != -1 && votedFor_ != req.candidate_id)) {
        resp.term = currentTerm_;
        resp.vote_granted = false;
        return;
    }
    if (req.term > currentTerm_) {
        changeState(NodeState::Follower);
        currentTerm_ = req.term;
        votedFor_ = -1;
    }
    if (!isLogUpToDate(req.last_log_term, req.last_log_index)) {
        resp.term = currentTerm_;
        resp.vote_granted = false;
        return;
    }
    votedFor_ = req.candidate_id;
    electionDeadline_ = std::chrono::steady_clock::now() + RandomizedElectionTimeout();
    resp.term = currentTerm_;
    resp.vote_granted = true;
}

void Raft::HandleAppendEntries(const AppendEntriesRequest& req, AppendEntriesResponse& resp) {
    std::lock_guard<std::mutex> lk(mu_);

    auto deferPersist = [&]{ persist(); };
    struct Guard { std::function<void()> fn; ~Guard(){ fn(); } };
    Guard g{deferPersist};

    if (req.term < currentTerm_) {
        resp.term = currentTerm_;
        resp.success = false;
        return;
    }
    if (req.term > currentTerm_) {
        currentTerm_ = req.term;
        votedFor_ = -1;
    }
    changeState(NodeState::Follower);
    electionDeadline_ = std::chrono::steady_clock::now() + RandomizedElectionTimeout();

    if (req.prev_log_index < getFirstLog().index) {
        resp.term = 0;
        resp.success = false;
        return;
    }
    if (!matchLog(req.prev_log_term, req.prev_log_index)) {
        resp.term = currentTerm_;
        resp.success = false;
        int lastIndex = getLastLog().index;
        if (lastIndex < req.prev_log_index) {
            resp.conflict_term = -1;
            resp.conflict_index = lastIndex + 1;
        } else {
            int firstIndex = getFirstLog().index;
            resp.conflict_term = logs_[req.prev_log_index - firstIndex].term;
            int idx = req.prev_log_index - 1;
            while (idx >= firstIndex && logs_[idx - firstIndex].term == resp.conflict_term) {
                --idx;
            }
            resp.conflict_index = idx;
        }
        return;
    }

    int firstIndex = getFirstLog().index;
    for (size_t i = 0; i < req.entries.size(); ++i) {
        const auto& entry = req.entries[i];
        if (entry.index - firstIndex >= static_cast<int>(logs_.size()) ||
            logs_[entry.index - firstIndex].term != entry.term) {
            logs_.resize(entry.index - firstIndex);
            for (size_t j = i; j < req.entries.size(); ++j) {
                logs_.push_back(req.entries[j]);
            }
            break;
        }
    }

    advanceCommitIndexForFollower(req.leader_commit);
    resp.term = currentTerm_;
    resp.success = true;
}

void Raft::HandleInstallSnapshot(const InstallSnapshotRequest& req, InstallSnapshotResponse& resp) {
    std::lock_guard<std::mutex> lk(mu_);

    resp.term = currentTerm_;
    if (req.term < currentTerm_) return;
    if (req.term > currentTerm_) {
        currentTerm_ = req.term;
        votedFor_ = -1;
        persist();
    }
    changeState(NodeState::Follower);
    electionDeadline_ = std::chrono::steady_clock::now() + RandomizedElectionTimeout();

    if (req.last_included_index <= commitIndex_) return;

    // Send snapshot to apply channel in a detached thread
    auto ch = applyCh_;
    ApplyMsg msg;
    msg.snapshot_valid = true;
    msg.snapshot       = req.data;
    msg.snapshot_term  = req.last_included_term;
    msg.snapshot_index = req.last_included_index;
    ch->push(std::move(msg));
}

// ── Internal log helpers ─────────────────────────────────────

bool Raft::isLogUpToDate(int term, int index) const {
    const auto& last = getLastLog();
    return term > last.term || (term == last.term && index >= last.index);
}

bool Raft::matchLog(int term, int index) const {
    return index <= getLastLog().index &&
           logs_[index - getFirstLog().index].term == term;
}

Entry Raft::appendNewEntry(const std::vector<uint8_t>& command) {
    const auto& last = getLastLog();
    Entry e;
    e.index = last.index + 1;
    e.term  = currentTerm_;
    e.command = command;
    logs_.push_back(e);
    matchIndex_[me_] = e.index;
    nextIndex_[me_]  = e.index + 1;
    persist();
    return e;
}

// ── State management ─────────────────────────────────────────

void Raft::changeState(NodeState newState) {
    if (state_ == newState) return;
    state_ = newState;
    switch (newState) {
        case NodeState::Follower:
            heartbeatDeadline_ = {};
            electionDeadline_ = std::chrono::steady_clock::now() + RandomizedElectionTimeout();
            break;
        case NodeState::Candidate:
            break;
        case NodeState::Leader: {
            int lastLogIdx = getLastLog().index;
            for (size_t i = 0; i < peers_.size(); ++i) {
                matchIndex_[i] = 0;
                nextIndex_[i]  = lastLogIdx + 1;
            }
            matchIndex_[me_] = lastLogIdx;
            electionDeadline_ = {};
            heartbeatDeadline_ = std::chrono::steady_clock::now() + StableHeartbeatTimeout();
            break;
        }
    }
}

// ── Commit index advancement ─────────────────────────────────

void Raft::advanceCommitIndexForLeader() {
    int n = static_cast<int>(matchIndex_.size());
    std::vector<int> srt(matchIndex_);
    insertion_sort_desc(srt);  // descending
    int newCommitIndex = srt[n - (n / 2 + 1)];
    if (newCommitIndex > commitIndex_) {
        if (matchLog(currentTerm_, newCommitIndex)) {
            commitIndex_ = newCommitIndex;
            applyCond_.notify_all();
        }
    }
}

void Raft::advanceCommitIndexForFollower(int leaderCommit) {
    int newCommitIndex = std::min(leaderCommit, getLastLog().index);
    if (newCommitIndex > commitIndex_) {
        commitIndex_ = newCommitIndex;
        applyCond_.notify_all();
    }
}

// ── Request builders ─────────────────────────────────────────

RequestVoteRequest Raft::genRequestVoteRequest() const {
    return {currentTerm_, me_, getLastLog().index, getLastLog().term};
}

AppendEntriesRequest Raft::genAppendEntriesRequest(int prevLogIndex) const {
    int firstIndex = getFirstLog().index;
    AppendEntriesRequest req;
    req.term         = currentTerm_;
    req.leader_id    = me_;
    req.prev_log_index = prevLogIndex;
    req.prev_log_term  = logs_[prevLogIndex - firstIndex].term;
    req.leader_commit  = commitIndex_;
    // Copy entries from prevLogIndex+1 onward
    auto begin = logs_.begin() + (prevLogIndex + 1 - firstIndex);
    req.entries.assign(begin, logs_.end());
    return req;
}

InstallSnapshotRequest Raft::genInstallSnapshotRequest() const {
    return {currentTerm_, me_,
            getFirstLog().index, getFirstLog().term,
            persister_->ReadSnapshot()};
}

// ── Election ─────────────────────────────────────────────────

void Raft::StartElection() {
    auto request = genRequestVoteRequest();
    auto grantedVotes = std::make_shared<std::atomic<int>>(1);
    votedFor_ = me_;
    persist();

    auto self = shared_from_this();
    for (size_t peer = 0; peer < peers_.size(); ++peer) {
        if (static_cast<int>(peer) == me_) continue;

        std::thread([self, request, peer, grantedVotes]() {
            RequestVoteResponse response;
            bool ok = false;
            self->sendRequestVote(static_cast<int>(peer), request, response, ok);
            if (!ok) return;

            std::lock_guard<std::mutex> lk(self->mu_);
            if (self->currentTerm_ == request.term &&
                self->state_ == NodeState::Candidate) {
                if (response.vote_granted) {
                    int votes = grantedVotes->fetch_add(1) + 1;
                    if (votes > static_cast<int>(self->peers_.size()) / 2) {
                        self->changeState(NodeState::Leader);
                        self->BroadcastHeartbeat(true);
                    }
                } else if (response.term > self->currentTerm_) {
                    self->changeState(NodeState::Follower);
                    self->currentTerm_ = response.term;
                    self->votedFor_ = -1;
                    self->persist();
                }
            }
        }).detach();
    }
}

// ── Heartbeat / replication ──────────────────────────────────

void Raft::BroadcastHeartbeat(bool /*isHeartbeat*/) {
    // Signal replicator threads — they handle both replication and heartbeats.
    // NOTE: We must NOT lock replicatorMu_ here because BroadcastHeartbeat is called
    // while holding mu_ (from Start() and StartElection()), and the replicator thread
    // locks replicatorMu_ while its wait_for predicate locks mu_ — classic deadlock.
    // notify_one() does not require holding the CV's mutex.
    for (size_t peer = 0; peer < peers_.size(); ++peer) {
        if (static_cast<int>(peer) == me_) continue;
        replicatorCv_[peer]->notify_one();
    }
}

bool Raft::replicateOneRound(int peer) {
    // Prepare request in a single lock scope to avoid race conditions
    // with snapshot compaction between multiple lock/unlock pairs.
    bool useSnapshot = false;
    AppendEntriesRequest aeReq;
    InstallSnapshotRequest snapReq;

    {
        std::lock_guard<std::mutex> lk(mu_);
        if (state_ != NodeState::Leader) return false;

        int prevLogIndex = nextIndex_[peer] - 1;
        useSnapshot = (prevLogIndex < getFirstLog().index);

        if (useSnapshot) {
            snapReq = genInstallSnapshotRequest();
        } else {
            aeReq = genAppendEntriesRequest(prevLogIndex);
        }
    }

    if (useSnapshot) {
        InstallSnapshotResponse resp;
        bool ok = false;
        sendInstallSnapshot(peer, snapReq, resp, ok);
        if (ok) {
            std::lock_guard<std::mutex> lk(mu_);
            handleInstallSnapshotResponse(peer, snapReq, resp);
        }
        return ok;
    } else {
        AppendEntriesResponse resp;
        bool ok = false;
        sendAppendEntries(peer, aeReq, resp, ok);
        if (ok) {
            std::lock_guard<std::mutex> lk(mu_);
            handleAppendEntriesResponse(peer, aeReq, resp);
        }
        return ok;
    }
}

// ── Response handlers ────────────────────────────────────────

void Raft::handleAppendEntriesResponse(int peer, const AppendEntriesRequest& req,
                                       const AppendEntriesResponse& resp) {
    if (state_ != NodeState::Leader || currentTerm_ != req.term) return;

    if (resp.success) {
        matchIndex_[peer] = req.prev_log_index + static_cast<int>(req.entries.size());
        nextIndex_[peer]  = matchIndex_[peer] + 1;
        advanceCommitIndexForLeader();
    } else {
        if (resp.term > currentTerm_) {
            changeState(NodeState::Follower);
            currentTerm_ = resp.term;
            votedFor_ = -1;
            persist();
        } else if (resp.term == currentTerm_) {
            nextIndex_[peer] = resp.conflict_index;
            if (resp.conflict_term != -1) {
                int firstIndex = getFirstLog().index;
                for (int i = req.prev_log_index; i >= firstIndex; --i) {
                    if (logs_[i - firstIndex].term == resp.conflict_term) {
                        nextIndex_[peer] = i + 1;
                        break;
                    }
                }
            }
        }
    }
}

void Raft::handleInstallSnapshotResponse(int peer, const InstallSnapshotRequest& req,
                                         const InstallSnapshotResponse& resp) {
    if (state_ != NodeState::Leader || currentTerm_ != req.term) return;

    if (resp.term > currentTerm_) {
        changeState(NodeState::Follower);
        currentTerm_ = resp.term;
        votedFor_ = -1;
        persist();
    } else {
        matchIndex_[peer] = req.last_included_index;
        nextIndex_[peer]  = req.last_included_index + 1;
    }
}

// ── RPC senders ──────────────────────────────────────────────

void Raft::sendRequestVote(int server, const RequestVoteRequest& req,
                           RequestVoteResponse& resp, bool& ok) {
    ok = peers_[server]->RequestVote(req, resp);
}

void Raft::sendAppendEntries(int server, const AppendEntriesRequest& req,
                             AppendEntriesResponse& resp, bool& ok) {
    ok = peers_[server]->AppendEntries(req, resp);
}

void Raft::sendInstallSnapshot(int server, const InstallSnapshotRequest& req,
                               InstallSnapshotResponse& resp, bool& ok) {
    ok = peers_[server]->InstallSnapshot(req, resp);
}

// ── Replicator need check ────────────────────────────────────

bool Raft::needReplicating(int peer) const {
    // Caller must hold mu_
    return state_ == NodeState::Leader &&
           matchIndex_[peer] < getLastLog().index;
}

// ── Start (client API) ──────────────────────────────────────

Raft::StartResult Raft::Start(const std::vector<uint8_t>& command) {
    std::lock_guard<std::mutex> lk(mu_);
    if (state_ != NodeState::Leader) {
        return {-1, -1, false};
    }
    auto newLog = appendNewEntry(command);
    BroadcastHeartbeat(false);
    return {newLog.index, newLog.term, true};
}

// ── Kill ─────────────────────────────────────────────────────

void Raft::Kill() {
    dead_.store(true, std::memory_order_relaxed);
    applyCh_->close();
    // Wake all replicator threads
    for (size_t i = 0; i < replicatorMu_.size(); ++i) {
        if (i != static_cast<size_t>(me_)) {
            std::lock_guard<std::mutex> lk(*replicatorMu_[i]);
            replicatorCv_[i]->notify_all();
        }
    }
    // Wake applier
    applyCond_.notify_all();

    // Join threads (safe: detach on failure to avoid std::terminate)
    auto safeJoin = [](std::thread& t) {
        try { if (t.joinable()) t.join(); } catch (...) { t.detach(); }
    };
    safeJoin(tickerThread_);
    safeJoin(applierThread_);
    for (auto& t : replicatorThreads_) {
        safeJoin(t);
    }
}

// ── Background: ticker ───────────────────────────────────────

void Raft::ticker() {
    while (!killed()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        std::unique_lock<std::mutex> lk(mu_);
        auto now = std::chrono::steady_clock::now();

        // Election timer
        if (electionDeadline_ != std::chrono::steady_clock::time_point{} &&
            now >= electionDeadline_) {
            changeState(NodeState::Candidate);
            currentTerm_ += 1;
            StartElection();
            electionDeadline_ = now + RandomizedElectionTimeout();
        }

        // Heartbeat is handled by replicator threads via wait_for timeout,
        // so no explicit heartbeat logic needed in ticker.
    }
}

// ── Background: applier ──────────────────────────────────────

void Raft::applier() {
    while (!killed()) {
        std::vector<Entry> toApply;
        int appliedUpTo = 0;
        {
            std::unique_lock<std::mutex> lk(mu_);
            applyCond_.wait(lk, [&] {
                return lastApplied_ < commitIndex_ || killed();
            });
            if (killed()) break;
            if (lastApplied_ >= commitIndex_) continue;

            int firstIndex = getFirstLog().index;
            int ci = commitIndex_;
            int la = lastApplied_;
            appliedUpTo = ci;
            for (int i = la + 1; i <= ci; ++i) {
                toApply.push_back(logs_[i - firstIndex]);
            }
        }

        for (const auto& entry : toApply) {
            ApplyMsg msg;
            msg.command_valid = true;
            msg.command       = entry.command;
            msg.command_term  = entry.term;
            msg.command_index = entry.index;
            applyCh_->push(std::move(msg));
        }

        {
            std::lock_guard<std::mutex> lk(mu_);
            lastApplied_ = std::max(lastApplied_, appliedUpTo);
        }
    }
}

// ── Background: replicator (one per peer) ────────────────────

void Raft::replicator(int peer) {
    while (!killed()) {
        {
            std::unique_lock<std::mutex> lk(*replicatorMu_[peer]);
            // Use wait_for with heartbeat timeout so this thread also sends
            // periodic empty heartbeats when there's nothing to replicate.
            replicatorCv_[peer]->wait_for(lk, std::chrono::milliseconds(100), [&] {
                std::lock_guard<std::mutex> rlk(mu_);
                return needReplicating(peer) || killed();
            });
            if (killed()) break;
        }
        // Check if we should send (either needs replication or is leader sending heartbeat)
        bool isLeader = false;
        {
            std::lock_guard<std::mutex> rlk(mu_);
            isLeader = (state_ == NodeState::Leader);
        }
        if (isLeader && !killed()) {
            replicateOneRound(peer);
        }
    }
}

// ── Factory: Make ────────────────────────────────────────────

Raft::~Raft() {
    Kill();
    auto safeJoin = [](std::thread& t) {
        try { if (t.joinable()) t.join(); } catch (...) { t.detach(); }
    };
    safeJoin(tickerThread_);
    safeJoin(applierThread_);
    for (auto& t : replicatorThreads_) {
        safeJoin(t);
    }
}

std::shared_ptr<Raft> Raft::Make(
    std::vector<std::shared_ptr<RaftPeer>> peers,
    int me,
    std::shared_ptr<Persister> persister,
    std::shared_ptr<BlockingQueue<ApplyMsg>> applyCh)
{
    // Use raw new + shared_ptr because constructor is private
    auto rf = std::shared_ptr<Raft>(new Raft());
    rf->peers_     = std::move(peers);
    rf->persister_ = std::move(persister);
    rf->me_        = me;
    rf->dead_.store(false);
    rf->applyCh_   = std::move(applyCh);
    rf->state_     = NodeState::Follower;
    rf->currentTerm_ = 0;
    rf->votedFor_  = -1;
    rf->logs_.resize(1);  // one dummy entry (index=0, term=0)

    int n = static_cast<int>(rf->peers_.size());
    rf->nextIndex_.resize(n, 0);
    rf->matchIndex_.resize(n, 0);
    rf->replicatorMu_.resize(n);
    rf->replicatorCv_.resize(n);
    for (int i = 0; i < n; ++i) {
        rf->replicatorMu_[i] = std::make_unique<std::mutex>();
        rf->replicatorCv_[i] = std::make_unique<std::condition_variable>();
    }

    // Restore from persisted state
    rf->readPersist(rf->persister_->ReadRaftState());

    int lastLogIdx = rf->getLastLog().index;
    for (int i = 0; i < n; ++i) {
        rf->matchIndex_[i] = 0;
        rf->nextIndex_[i]  = lastLogIdx + 1;
    }

    // If there's a snapshot, send it to the applier so the service layer
    // can restore its state before applying new entries.
    {
        auto snap = rf->persister_->ReadSnapshot();
        int firstIndex = rf->getFirstLog().index;
        if (!snap.empty() && firstIndex > 0) {
            ApplyMsg msg;
            msg.snapshot_valid  = true;
            msg.snapshot        = std::move(snap);
            msg.snapshot_index  = firstIndex;
            msg.snapshot_term   = rf->getFirstLog().term;
            rf->applyCh_->push(std::move(msg));
            rf->lastApplied_ = firstIndex;
        }
    }

    // Initialize timers
    rf->electionDeadline_ = std::chrono::steady_clock::now() + RandomizedElectionTimeout();
    rf->heartbeatDeadline_ = std::chrono::steady_clock::now() + StableHeartbeatTimeout();

    // Start background threads
    rf->tickerThread_  = std::thread(&Raft::ticker, rf.get());
    rf->applierThread_ = std::thread(&Raft::applier, rf.get());
    rf->replicatorThreads_.resize(n);
    for (int i = 0; i < n; ++i) {
        if (i != me) {
            rf->replicatorThreads_[i] = std::thread(&Raft::replicator, rf.get(), i);
        }
    }

    return rf;
}

} // namespace raft
