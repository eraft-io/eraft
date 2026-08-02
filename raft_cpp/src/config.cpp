// config.cpp — Test configuration framework implementation
// Corresponds to Go: config.go

#include "raft/config.h"

#include <cstring>
#include <iostream>
#include <sstream>

namespace raft {

// ── Helpers ──────────────────────────────────────────────────

static std::mt19937& globalRng() {
    static thread_local std::mt19937 rng(std::random_device{}());
    return rng;
}

static int randInt(int lo, int hi) {
    std::uniform_int_distribution<int> dist(lo, hi - 1);
    return dist(globalRng());
}

// Encode an int as bytes (for test commands)
static std::vector<uint8_t> intToBytes(int v) {
    std::vector<uint8_t> b(sizeof(v));
    std::memcpy(b.data(), &v, sizeof(v));
    return b;
}

static int bytesToInt(const std::vector<uint8_t>& b) {
    if (b.size() < sizeof(int)) return 0;
    int v;
    std::memcpy(&v, b.data(), sizeof(v));
    return v;
}

// ── Apply thread function ────────────────────────────────────

void Config::applyThreadStatic(Config* cfg, int serverIdx,
                        std::shared_ptr<BlockingQueue<ApplyMsg>> ch,
                        bool doSnapshot) {
    int lastApplied = 0;
    ApplyMsg m;
    while (ch->pop(m)) {
        if (m.snapshot_valid) {
            std::lock_guard<std::mutex> lk(cfg->mu_);  // access to cfg internals
            if (cfg->rafts(serverIdx) &&
                cfg->rafts(serverIdx)->CondInstallSnapshot(
                    m.snapshot_term, m.snapshot_index, m.snapshot)) {
                cfg->logs_[serverIdx].clear();
                // Decode snapshot value
                int v = bytesToInt(m.snapshot);
                cfg->logs_[serverIdx][m.snapshot_index] = intToBytes(v);
                lastApplied = m.snapshot_index;
            }
        } else if (m.command_valid && m.command_index > lastApplied) {
            std::string errMsg;
            {
                std::lock_guard<std::mutex> lk(cfg->mu_);
                errMsg = cfg->checkLogs(serverIdx, m);
            }
            if (m.command_index > 1) {
                std::lock_guard<std::mutex> lk(cfg->mu_);
                bool prevOk = cfg->logs_[serverIdx].count(m.command_index - 1) > 0;
                if (!prevOk) {
                    std::ostringstream oss;
                    oss << "server " << serverIdx
                        << " apply out of order " << m.command_index;
                    errMsg = oss.str();
                }
            }
            if (!errMsg.empty()) {
                std::lock_guard<std::mutex> lk(cfg->mu_);
                cfg->applyErr_[serverIdx] = errMsg;
            }
            lastApplied = m.command_index;

            if (doSnapshot && ((m.command_index + 1) % 10 == 0)) {
                if (cfg->rafts(serverIdx)) {
                    cfg->rafts(serverIdx)->Snapshot(m.command_index,
                                                    intToBytes(bytesToInt(m.command)));
                }
            }
        }
    }
}

// ── Config constructor ───────────────────────────────────────

Config::Config(int n, bool unreliable, bool snapshot)
    : n_(n), unreliable_(unreliable), snapshot_(snapshot),
      start_(std::chrono::steady_clock::now())
{
    rafts_.resize(n);
    connected_.resize(n, false);
    saved_.resize(n);
    peers_.resize(n);
    applyErr_.resize(n);
    logs_.resize(n);
    applyChs_.resize(n);
    applyThreads_.resize(n);

    for (int i = 0; i < n; ++i) {
        peers_[i].resize(n);
        for (int j = 0; j < n; ++j) {
            peers_[i][j] = std::make_shared<InMemPeer>();
        }
    }

    for (int i = 0; i < n; ++i) {
        start1(i);
    }
    for (int i = 0; i < n; ++i) {
        connect(i);
    }
}

Config::~Config() {
    cleanup();
}

// ── start1 ───────────────────────────────────────────────────

void Config::start1(int i) {
    crash1(i);

    // Fresh persister
    if (saved_[i]) {
        saved_[i] = saved_[i]->Copy();
    } else {
        saved_[i] = std::make_shared<Persister>();
    }

    // Create apply channel
    applyChs_[i] = std::make_shared<BlockingQueue<ApplyMsg>>();

    // Build peer list for this server
    std::vector<std::shared_ptr<RaftPeer>> peerList(n_);
    for (int j = 0; j < n_; ++j) {
        peerList[j] = peers_[i][j];
    }

    auto rf = Raft::Make(peerList, i, saved_[i], applyChs_[i]);

    // Wire up: other servers' peers that point to i should target this new rf
    for (int j = 0; j < n_; ++j) {
        peers_[j][i]->setTarget(rf);
    }

    {
        std::lock_guard<std::mutex> lk(mu_);
        rafts_[i] = rf;
    }

    // Start apply thread
    applyThreads_[i] = std::thread(&Config::applyThreadStatic, this, i, applyChs_[i], snapshot_);
}

// ── crash1 ───────────────────────────────────────────────────

void Config::crash1(int i) {
    disconnect(i);

    std::shared_ptr<Raft> rf;
    {
        std::lock_guard<std::mutex> lk(mu_);
        if (saved_[i]) {
            saved_[i] = saved_[i]->Copy();
        }
        rf = rafts_[i];
        rafts_[i] = nullptr;
    }

    if (rf) {
        rf->Kill();
    }

    // Close apply channel to stop apply thread
    if (applyChs_[i]) {
        applyChs_[i]->close();
        if (applyThreads_[i].joinable()) {
            applyThreads_[i].join();
        }
        applyChs_[i].reset();
    }

    if (saved_[i]) {
        auto state    = saved_[i]->ReadRaftState();
        auto snapshot = saved_[i]->ReadSnapshot();
        saved_[i] = std::make_shared<Persister>();
        saved_[i]->SaveStateAndSnapshot(state, snapshot);
    }
}

// ── connect / disconnect ─────────────────────────────────────

void Config::connect(int i) {
    connected_[i] = true;
    for (int j = 0; j < n_; ++j) {
        if (connected_[j]) {
            peers_[i][j]->setEnabled(true);  // i -> j
            peers_[j][i]->setEnabled(true);  // j -> i
        }
    }
}

void Config::disconnect(int i) {
    connected_[i] = false;
    for (int j = 0; j < n_; ++j) {
        peers_[i][j]->setEnabled(false);
        peers_[j][i]->setEnabled(false);
    }
}

// ── checkLogs ────────────────────────────────────────────────

std::string Config::checkLogs(int i, const ApplyMsg& m) {
    // Caller must hold mu_
    const auto& v = m.command;
    for (int j = 0; j < n_; ++j) {
        auto it = logs_[j].find(m.command_index);
        if (it != logs_[j].end() && it->second != v) {
            std::ostringstream oss;
            oss << "commit index=" << m.command_index
                << " server=" << i << " != server=" << j;
            return oss.str();
        }
    }
    logs_[i][m.command_index] = v;
    if (m.command_index > maxIndex_) {
        maxIndex_ = m.command_index;
    }
    return "";
}

// ── checkOneLeader ───────────────────────────────────────────

int Config::checkOneLeader() {
    for (int iter = 0; iter < 10; ++iter) {
        int ms = 450 + (randInt(0, 100));
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));

        std::map<int, std::vector<int>> leaders;
        for (int i = 0; i < n_; ++i) {
            if (connected_[i] && rafts_[i]) {
                auto [term, isLeader] = rafts_[i]->GetState();
                if (isLeader) {
                    leaders[term].push_back(i);
                }
            }
        }

        int lastTermWithLeader = -1;
        for (auto& [term, leaderList] : leaders) {
            if (leaderList.size() > 1) {
                std::ostringstream oss;
                oss << "term " << term << " has " << leaderList.size() << " (>1) leaders";
                throw std::runtime_error(oss.str());
            }
            if (term > lastTermWithLeader) {
                lastTermWithLeader = term;
            }
        }
        if (!leaders.empty()) {
            return leaders[lastTermWithLeader][0];
        }
    }
    throw std::runtime_error("expected one leader, got none");
}

// ── checkTerms ───────────────────────────────────────────────

int Config::checkTerms() {
    int term = -1;
    for (int i = 0; i < n_; ++i) {
        if (connected_[i] && rafts_[i]) {
            auto [t, _] = rafts_[i]->GetState();
            if (term == -1) {
                term = t;
            } else if (term != t) {
                throw std::runtime_error("servers disagree on term");
            }
        }
    }
    return term;
}

// ── checkNoLeader ────────────────────────────────────────────

void Config::checkNoLeader() {
    for (int i = 0; i < n_; ++i) {
        if (connected_[i] && rafts_[i]) {
            auto [_, isLeader] = rafts_[i]->GetState();
            if (isLeader) {
                std::ostringstream oss;
                oss << "expected no leader, but " << i << " claims to be leader";
                throw std::runtime_error(oss.str());
            }
        }
    }
}

// ── nCommitted ───────────────────────────────────────────────

std::pair<int, std::vector<uint8_t>> Config::nCommitted(int index) {
    std::lock_guard<std::mutex> lk(mu_);
    int count = 0;
    std::vector<uint8_t> cmd;
    for (int i = 0; i < n_; ++i) {
        if (!applyErr_[i].empty()) {
            throw std::runtime_error(applyErr_[i]);
        }
        auto it = logs_[i].find(index);
        if (it != logs_[i].end()) {
            if (count > 0 && cmd != it->second) {
                std::ostringstream oss;
                oss << "committed values do not match: index " << index;
                throw std::runtime_error(oss.str());
            }
            cmd = it->second;
            ++count;
        }
    }
    return {count, cmd};
}

// ── wait ─────────────────────────────────────────────────────

std::vector<uint8_t> Config::wait(int index, int n, int startTerm) {
    int to = 10;
    for (int iter = 0; iter < 30; ++iter) {
        auto [nd, cmd] = nCommitted(index);
        if (nd >= n) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(to));
        if (to < 1000) to *= 2;
        if (startTerm > -1) {
            for (auto& rf : rafts_) {
                if (rf) {
                    auto [t, _] = rf->GetState();
                    if (t > startTerm) {
                        // Someone moved on; return sentinel
                        return {};
                    }
                }
            }
        }
    }
    auto [nd, cmd] = nCommitted(index);
    if (nd < n) {
        std::ostringstream oss;
        oss << "only " << nd << " decided for index " << index
            << "; wanted " << n;
        throw std::runtime_error(oss.str());
    }
    return cmd;
}

// ── one ──────────────────────────────────────────────────────

int Config::one(const std::vector<uint8_t>& cmd, int expectedServers, bool retry) {
    auto t0 = std::chrono::steady_clock::now();
    int starts = 0;
    while (std::chrono::steady_clock::now() - t0 < std::chrono::seconds(30)) {
        int index = -1;
        for (int si = 0; si < n_; ++si) {
            starts = (starts + 1) % n_;
            std::shared_ptr<Raft> rf;
            {
                std::lock_guard<std::mutex> lk(mu_);
                if (connected_[starts]) {
                    rf = rafts_[starts];
                }
            }
            if (rf) {
                auto result = rf->Start(cmd);
                if (result.isLeader) {
                    index = result.index;
                    break;
                }
            }
        }

        if (index != -1) {
            auto t1 = std::chrono::steady_clock::now();
            while (std::chrono::steady_clock::now() - t1 < std::chrono::seconds(5)) {
                auto [nd, cmd1] = nCommitted(index);
                if (nd > 0 && nd >= expectedServers) {
                    if (cmd1 == cmd) return index;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            if (!retry) {
                throw std::runtime_error("one() failed to reach agreement");
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    throw std::runtime_error("one() failed to reach agreement");
}

// ── LogSize ──────────────────────────────────────────────────

int Config::LogSize() const {
    int maxsize = 0;
    for (int i = 0; i < n_; ++i) {
        if (saved_[i]) {
            int s = static_cast<int>(saved_[i]->RaftStateSize());
            if (s > maxsize) maxsize = s;
        }
    }
    return maxsize;
}

// ── cleanup ──────────────────────────────────────────────────

void Config::cleanup() {
    // Kill all Raft instances first (without holding mu_ to avoid deadlock with ticker thread)
    for (int i = 0; i < n_; ++i) {
        std::shared_ptr<Raft> rf;
        {
            std::lock_guard<std::mutex> lk(mu_);
            rf = std::move(rafts_[i]);
        }
        if (rf) {
            rf->Kill();
        }
    }
    for (int i = 0; i < n_; ++i) {
        if (applyChs_[i]) {
            applyChs_[i]->close();
        }
        if (applyThreads_[i].joinable()) {
            applyThreads_[i].join();
        }
    }
}

// ── begin / end ──────────────────────────────────────────────

void Config::begin(const std::string& description) {
    std::cout << description << " ..." << std::endl;
    t0_ = std::chrono::steady_clock::now();
    cmds0_ = 0;
    maxIndex0_ = maxIndex_;
}

void Config::end() {
    auto elapsed = std::chrono::steady_clock::now() - t0_;
    double secs = std::chrono::duration<double>(elapsed).count();
    int ncmds = maxIndex_ - maxIndex0_;
    std::cout << "  ... Passed -- "
              << secs << "s  "
              << n_ << " peers  "
              << ncmds << " cmds" << std::endl;
}

} // namespace raft
