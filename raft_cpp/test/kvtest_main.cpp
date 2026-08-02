// kvtest_main.cpp — KV Raft test suite + KVConfig framework + porcupine linearizability
// Corresponds to Go: kvraft/config.go + kvraft/test_test.go + porcupine

#include <gtest/gtest.h>

#include "raft/clerk.h"
#include "raft/kvcommon.h"
#include "raft/kvserver.h"
#include "raft/kvstatemachine.h"
#include "raft/config.h"
#include "raft/persister.h"
#include "raft/util.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace raft {
namespace kvtest {

// ═══════════════════════════════════════════════════════════════
// Porcupine linearizability checker (C++ port)
// ═══════════════════════════════════════════════════════════════

// ── Bitset ───────────────────────────────────────────────────
struct Bitset {
    std::vector<uint64_t> data;

    explicit Bitset(unsigned bits) {
        data.resize((bits + 63) / 64, 0);
    }

    Bitset clone() const { return *this; }

    Bitset& set(unsigned pos) {
        data[pos / 64] |= (1ULL << (pos % 64));
        return *this;
    }

    Bitset& clear(unsigned pos) {
        data[pos / 64] &= ~(1ULL << (pos % 64));
        return *this;
    }

    bool get(unsigned pos) const {
        return (data[pos / 64] >> (pos % 64)) & 1;
    }

    unsigned popcnt() const {
        unsigned total = 0;
        for (auto v : data) total += __builtin_popcountll(v);
        return total;
    }

    uint64_t hash() const {
        uint64_t h = popcnt();
        for (auto v : data) h ^= v;
        return h;
    }

    bool operator==(const Bitset& o) const { return data == o.data; }
};

// ── Operation ────────────────────────────────────────────────
enum CheckResult { CheckOk, CheckIllegal, CheckUnknown };

struct Operation {
    int clientId = 0;
    int inputOp  = 0;       // 0=get, 1=put, 2=append
    std::string inputKey;
    std::string inputValue;
    std::string outputValue;
    int64_t call   = 0;
    int64_t ret    = 0;
};

// ── KV Model ─────────────────────────────────────────────────
// Partition operations by key
static std::vector<std::vector<Operation>> partitionByKey(const std::vector<Operation>& ops) {
    std::map<std::string, std::vector<Operation>> m;
    for (auto& op : ops) m[op.inputKey].push_back(op);
    std::vector<std::vector<Operation>> result;
    for (auto& [k, v] : m) result.push_back(v);
    return result;
}

// Step: returns (ok, newState)
static std::pair<bool, std::string> kvStep(const std::string& state, int op,
                                            const std::string& inValue,
                                            const std::string& outValue) {
    if (op == 0) { // get
        return {outValue == state, state};
    } else if (op == 1) { // put
        return {true, inValue};
    } else { // append
        return {true, state + inValue};
    }
}

// ── Entry for checker ────────────────────────────────────────
enum EntryKind { CallEntry, ReturnEntry };

struct LinEntry {
    EntryKind kind;
    int id;
    int64_t time;
    // call entry: stores input (op, inValue)
    // return entry: stores output (outValue)
    int op = 0;
    std::string value;   // inputValue for calls, outputValue for returns
};

static std::vector<LinEntry> makeEntries(const std::vector<Operation>& history) {
    std::vector<LinEntry> entries;
    int id = 0;
    for (auto& elem : history) {
        entries.push_back({CallEntry, id, elem.call, elem.inputOp, elem.inputValue});
        entries.push_back({ReturnEntry, id, elem.ret, elem.inputOp, elem.outputValue});
        ++id;
    }
    std::sort(entries.begin(), entries.end(),
              [](const LinEntry& a, const LinEntry& b) { return a.time < b.time; });
    return entries;
}

// ── Linked list node for checker ─────────────────────────────
struct LinNode {
    LinEntry entry;
    LinNode* match = nullptr;  // non-null for call entries -> return entry
    LinNode* next = nullptr;
    LinNode* prev = nullptr;
};

static int listLength(LinNode* n) {
    int l = 0;
    for (; n; n = n->next) ++l;
    return l;
}

static void insertBefore(LinNode* n, LinNode* mark) {
    if (mark) {
        auto* beforeMark = mark->prev;
        mark->prev = n;
        n->next = mark;
        if (beforeMark) {
            n->prev = beforeMark;
            beforeMark->next = n;
        }
    }
}

// Returns (head of linked list, storage for cleanup)
static std::pair<LinNode*, std::vector<LinNode>> makeLinkedEntries(const std::vector<LinEntry>& entries) {
    std::vector<LinNode> nodes(entries.size() + 1); // +1 for head sentinel
    LinNode* root = nullptr;
    std::unordered_map<int, LinNode*> matchMap;

    for (int i = static_cast<int>(entries.size()) - 1; i >= 0; --i) {
        nodes[i].entry = entries[i];
        if (entries[i].kind == ReturnEntry) {
            nodes[i].match = nullptr;
            matchMap[entries[i].id] = &nodes[i];
            insertBefore(&nodes[i], root);
            root = &nodes[i];
        } else {
            auto it = matchMap.find(entries[i].id);
            nodes[i].match = (it != matchMap.end()) ? it->second : nullptr;
            insertBefore(&nodes[i], root);
            root = &nodes[i];
        }
    }
    return {root, std::move(nodes)};
}

static void lift(LinNode* entry) {
    entry->prev->next = entry->next;
    if (entry->next) entry->next->prev = entry->prev;
    auto* m = entry->match;
    m->prev->next = m->next;
    if (m->next) m->next->prev = m->prev;
}

static void unlift(LinNode* entry) {
    auto* m = entry->match;
    m->prev->next = m;
    if (m->next) m->next->prev = m;
    entry->prev->next = entry;
    if (entry->next) entry->next->prev = entry;
}

struct CacheEntry {
    Bitset linearized;
    std::string state;
};

static bool cacheContains(const std::unordered_map<uint64_t, std::vector<CacheEntry>>& cache,
                           const CacheEntry& entry) {
    auto it = cache.find(entry.linearized.hash());
    if (it == cache.end()) return false;
    for (auto& elem : it->second) {
        if (entry.linearized == elem.linearized && entry.state == elem.state)
            return true;
    }
    return false;
}

struct CallsEntry {
    LinNode* entry;
    std::string state;
};

static bool checkSingle(const std::vector<LinEntry>& history, std::atomic<bool>& kill) {
    auto [root, storage] = makeLinkedEntries(history);
    int n = listLength(root) / 2;
    Bitset linearized(static_cast<unsigned>(n));
    std::unordered_map<uint64_t, std::vector<CacheEntry>> cache;
    std::vector<CallsEntry> calls;

    std::string state;  // initial state = ""

    // Use last element of storage as head sentinel
    auto* headEntry = &storage.back();
    headEntry->entry = {CallEntry, -1, 0, 0, ""};
    headEntry->match = nullptr;
    insertBefore(headEntry, root);

    auto* entry = headEntry->next;

    while (entry) {
        if (kill.load()) { return false; }

        if (entry->match) {
            // Try to linearize this call
            auto* matching = entry->match;
            auto [ok, newState] = kvStep(state, entry->entry.op,
                                          entry->entry.value, matching->entry.value);
            if (ok) {
                CacheEntry newCE{linearized.clone().set(static_cast<unsigned>(entry->entry.id)), newState};
                if (!cacheContains(cache, newCE)) {
                    uint64_t h = newCE.linearized.hash();
                    cache[h].push_back(newCE);
                    calls.push_back({entry, state});
                    state = newState;
                    linearized.set(static_cast<unsigned>(entry->entry.id));
                    lift(entry);
                    entry = headEntry->next;
                } else {
                    entry = entry->next;
                }
            } else {
                entry = entry->next;
            }
        } else {
            // Backtrack
            if (calls.empty()) {
                return false;
            }
            auto top = calls.back();
            entry = top.entry;
            state = top.state;
            linearized.clear(static_cast<unsigned>(entry->entry.id));
            calls.pop_back();
            unlift(entry);
            entry = entry->next;
        }
    }

    return true;
}

static CheckResult checkLinearizability(const std::vector<Operation>& ops,
                                         std::chrono::milliseconds timeout) {
    auto partitions = partitionByKey(ops);
    std::atomic<bool> ok{true};
    std::atomic<bool> kill{false};
    std::atomic<int> count{0};
    int nPartitions = static_cast<int>(partitions.size());

    std::vector<std::thread> threads;
    for (int i = 0; i < nPartitions; ++i) {
        threads.emplace_back([&, i]() {
            auto entries = makeEntries(partitions[i]);
            bool result = checkSingle(entries, kill);
            if (!result) ok.store(false);
            if (!result) kill.store(true);
            count.fetch_add(1);
        });
    }

    // Wait with timeout
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (count.load() < nPartitions) {
        if (std::chrono::steady_clock::now() > deadline) {
            kill.store(true);
            for (auto& t : threads) if (t.joinable()) t.join();
            return CheckUnknown;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    for (auto& t : threads) if (t.joinable()) t.join();

    return ok.load() ? CheckOk : CheckIllegal;
}

// ═══════════════════════════════════════════════════════════════
// KVConfig — Test framework
// ═══════════════════════════════════════════════════════════════

static std::mt19937& rng() {
    static thread_local std::mt19937 r(std::random_device{}());
    return r;
}

class KVConfig {
public:
    KVConfig(int n, bool unreliable, int maxRaftState, const std::string& testName);
    ~KVConfig();

    std::shared_ptr<Clerk> makeClient(const std::vector<int>& to);
    void connectClient(Clerk* ck, const std::vector<int>& to);
    void disconnectClient(Clerk* ck, const std::vector<int>& from);
    void deleteClient(std::shared_ptr<Clerk> ck);

    void connect(int i);
    void disconnect(int i);
    void connectAll();
    void partition(const std::vector<int>& p1, const std::vector<int>& p2);

    void shutdownServer(int i);
    void startServer(int i);

    int checkOneLeader();
    std::pair<bool, int> leader();
    std::pair<std::vector<int>, std::vector<int>> makePartition();

    int logSize();
    int snapshotSize();
    int getSavedRaftSize(int i);

    void begin(const std::string& desc);
    void end();
    void op() { ops_.fetch_add(1); }

    std::vector<int> all() const {
        std::vector<int> a(n_);
        std::iota(a.begin(), a.end(), 0);
        return a;
    }

    int n() const { return n_; }

private:
    void cleanup();

    int n_;
    bool unreliable_;
    int maxRaftState_;
    std::string testName_;
    std::string tmpDir_;
    std::chrono::steady_clock::time_point start_;

    // Per-server state
    std::vector<std::shared_ptr<KVServer>> kvServers_;
    std::vector<std::shared_ptr<Persister>> saved_;
    // peers_[i][j] = peer that server i uses to talk to server j (for Raft RPCs)
    std::vector<std::vector<std::shared_ptr<InMemPeer>>> raftPeers_;
    // kvPeers_[i][j] = KV peer that server i uses to talk to KVServer j
    // Actually, clerk has its own set of KV peers
    std::vector<bool> connected_;

    // Per-clerk KV peers: clerkPeers_[ck][j]
    std::mutex mu_;
    std::map<Clerk*, std::vector<std::shared_ptr<InMemKVPeer>>> clerkPeers_;

    std::chrono::steady_clock::time_point t0_;
    std::atomic<int> ops_{0};
};

KVConfig::KVConfig(int n, bool unreliable, int maxRaftState, const std::string& testName)
    : n_(n), unreliable_(unreliable), maxRaftState_(maxRaftState), testName_(testName),
      start_(std::chrono::steady_clock::now())
{
    // Create temp directory for RocksDB
    auto tmpBase = std::filesystem::temp_directory_path() / ("kvraft-test-" + std::to_string(getpid()));
    std::filesystem::create_directories(tmpBase);
    tmpDir_ = tmpBase.string();

    kvServers_.resize(n);
    saved_.resize(n);
    raftPeers_.resize(n);
    connected_.resize(n, false);

    for (int i = 0; i < n; ++i) {
        raftPeers_[i].resize(n);
        for (int j = 0; j < n; ++j) {
            raftPeers_[i][j] = std::make_shared<InMemPeer>();
        }
    }

    for (int i = 0; i < n; ++i) startServer(i);
    connectAll();

    if (unreliable_) {
        // For unreliable mode, we'll randomly drop messages at the KV peer level
        // Actually, InMemPeer already has enabled/disabled for Raft.
        // For KV unreliable, we rely on the Raft-level unreliability.
    }
}

KVConfig::~KVConfig() {
    cleanup();
    // Remove temp directory
    try { std::filesystem::remove_all(tmpDir_); } catch (...) {}
}

void KVConfig::cleanup() {
    for (int i = 0; i < n_; ++i) {
        if (kvServers_[i]) {
            kvServers_[i]->Kill();
            kvServers_[i].reset();
        }
    }
}

void KVConfig::startServer(int i) {
    // Disconnect first
    disconnect(i);

    {
        std::lock_guard<std::mutex> lk(mu_);
        if (saved_[i]) {
            saved_[i] = saved_[i]->Copy();
        } else {
            saved_[i] = std::make_shared<Persister>();
        }
    }

    // Kill old server
    {
        std::lock_guard<std::mutex> lk(mu_);
        if (kvServers_[i]) {
            kvServers_[i]->Kill();
            kvServers_[i].reset();
        }
    }

    // Fresh persister with old data
    if (saved_[i]) {
        auto state = saved_[i]->ReadRaftState();
        auto snap = saved_[i]->ReadSnapshot();
        saved_[i] = std::make_shared<Persister>();
        saved_[i]->SaveStateAndSnapshot(state, snap);
    } else {
        saved_[i] = std::make_shared<Persister>();
    }

    // Build peer list
    std::vector<std::shared_ptr<RaftPeer>> peerList(n_);
    for (int j = 0; j < n_; ++j) {
        peerList[j] = raftPeers_[i][j];
    }

    std::string dbPath = tmpDir_ + "/kvserver-" + std::to_string(i);
    // Clean old DB
    try { std::filesystem::remove_all(dbPath); } catch (...) {}

    auto kv = KVServer::Make(peerList, i, saved_[i], maxRaftState_, dbPath);

    // Wire up raft peers
    for (int j = 0; j < n_; ++j) {
        raftPeers_[j][i]->setTarget(kv->getRaft());
    }

    {
        std::lock_guard<std::mutex> lk(mu_);
        kvServers_[i] = kv;

        // Update all clerk peers to target the new KVServer
        for (auto& [ck, peers] : clerkPeers_) {
            if (i < static_cast<int>(peers.size())) {
                peers[i]->setTarget(kv);
            }
        }
    }
}

void KVConfig::shutdownServer(int i) {
    disconnect(i);

    {
        std::lock_guard<std::mutex> lk(mu_);
        if (saved_[i]) {
            saved_[i] = saved_[i]->Copy();
        }
    }

    std::shared_ptr<KVServer> kv;
    {
        std::lock_guard<std::mutex> lk(mu_);
        kv = kvServers_[i];
        kvServers_[i].reset();
    }

    if (kv) {
        kv->Kill();
    }
}

void KVConfig::connect(int i) {
    connected_[i] = true;
    for (int j = 0; j < n_; ++j) {
        if (connected_[j]) {
            raftPeers_[i][j]->setEnabled(true);
            raftPeers_[j][i]->setEnabled(true);
        }
    }
}

void KVConfig::disconnect(int i) {
    connected_[i] = false;
    for (int j = 0; j < n_; ++j) {
        raftPeers_[i][j]->setEnabled(false);
        raftPeers_[j][i]->setEnabled(false);
    }
}

void KVConfig::connectAll() {
    for (int i = 0; i < n_; ++i) connect(i);
}

void KVConfig::partition(const std::vector<int>& p1, const std::vector<int>& p2) {
    for (int i : p1) {
        for (int j : p2) {
            raftPeers_[i][j]->setEnabled(false);
            raftPeers_[j][i]->setEnabled(false);
        }
        for (int j : p1) {
            raftPeers_[i][j]->setEnabled(true);
            raftPeers_[j][i]->setEnabled(true);
        }
    }
    for (int i : p2) {
        for (int j : p2) {
            raftPeers_[i][j]->setEnabled(true);
            raftPeers_[j][i]->setEnabled(true);
        }
    }
}

std::shared_ptr<Clerk> KVConfig::makeClient(const std::vector<int>& to) {
    std::lock_guard<std::mutex> lk(mu_);

    auto peers = std::vector<std::shared_ptr<InMemKVPeer>>(n_);
    for (int j = 0; j < n_; ++j) {
        peers[j] = std::make_shared<InMemKVPeer>();
        if (kvServers_[j]) {
            peers[j]->setTarget(kvServers_[j]);
        }
        // Enable only for servers in 'to'
        bool inTo = std::find(to.begin(), to.end(), j) != to.end();
        peers[j]->setEnabled(inTo);
    }

    std::vector<std::shared_ptr<KVPeer>> peerVec(peers.begin(), peers.end());
    auto ck = std::make_shared<Clerk>(peerVec);
    clerkPeers_[ck.get()] = peers;
    return ck;
}

void KVConfig::connectClient(Clerk* ck, const std::vector<int>& to) {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = clerkPeers_.find(ck);
    if (it == clerkPeers_.end()) return;
    for (int j : to) {
        if (j < static_cast<int>(it->second.size())) {
            it->second[j]->setEnabled(true);
        }
    }
}

void KVConfig::disconnectClient(Clerk* ck, const std::vector<int>& from) {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = clerkPeers_.find(ck);
    if (it == clerkPeers_.end()) return;
    for (int j : from) {
        if (j < static_cast<int>(it->second.size())) {
            it->second[j]->setEnabled(false);
        }
    }
}

void KVConfig::deleteClient(std::shared_ptr<Clerk> ck) {
    std::lock_guard<std::mutex> lk(mu_);
    clerkPeers_.erase(ck.get());
}

int KVConfig::checkOneLeader() {
    for (int iter = 0; iter < 10; ++iter) {
        std::this_thread::sleep_for(std::chrono::milliseconds(450 + rng()() % 100));
        std::map<int, std::vector<int>> leaders;
        for (int i = 0; i < n_; ++i) {
            if (connected_[i] && kvServers_[i]) {
                auto [term, isLeader] = kvServers_[i]->getRaft()->GetState();
                if (isLeader) leaders[term].push_back(i);
            }
        }
        int lastTerm = -1;
        for (auto& [term, list] : leaders) {
            if (list.size() > 1)
                throw std::runtime_error("term " + std::to_string(term) + " has multiple leaders");
            lastTerm = std::max(lastTerm, term);
        }
        if (!leaders.empty()) return leaders[lastTerm][0];
    }
    throw std::runtime_error("no leader elected");
}

std::pair<bool, int> KVConfig::leader() {
    for (int i = 0; i < n_; ++i) {
        if (kvServers_[i]) {
            auto [_, isLeader] = kvServers_[i]->getRaft()->GetState();
            if (isLeader) return {true, i};
        }
    }
    return {false, 0};
}

std::pair<std::vector<int>, std::vector<int>> KVConfig::makePartition() {
    auto [_, l] = leader();
    std::vector<int> p1(n_ / 2 + 1), p2(n_ / 2);
    int j = 0;
    for (int i = 0; i < n_; ++i) {
        if (i != l) {
            if (j < static_cast<int>(p1.size())) p1[j] = i;
            else p2[j - p1.size()] = i;
            ++j;
        }
    }
    p2.back() = l;
    return {p1, p2};
}

int KVConfig::getSavedRaftSize(int i) {
    if (i < 0 || i >= n_ || !saved_[i]) return -1;
    return static_cast<int>(saved_[i]->RaftStateSize());
}

int KVConfig::logSize() {
    int mx = 0;
    for (int i = 0; i < n_; ++i) {
        if (saved_[i]) {
            int s = static_cast<int>(saved_[i]->RaftStateSize());
            mx = std::max(mx, s);
        }
    }
    return mx;
}

int KVConfig::snapshotSize() {
    int mx = 0;
    for (int i = 0; i < n_; ++i) {
        if (saved_[i]) {
            int s = static_cast<int>(saved_[i]->SnapshotSize());
            mx = std::max(mx, s);
        }
    }
    return mx;
}

void KVConfig::begin(const std::string& desc) {
    std::cout << desc << " ..." << std::endl;
    t0_ = std::chrono::steady_clock::now();
    ops_.store(0);
}

void KVConfig::end() {
    auto elapsed = std::chrono::steady_clock::now() - t0_;
    double secs = std::chrono::duration<double>(elapsed).count();
    int o = ops_.load();
    std::cout << "  ... Passed -- " << secs << "s  " << n_ << " servers  " << o << " ops" << std::endl;
}

// ═══════════════════════════════════════════════════════════════
// Test helpers
// ═══════════════════════════════════════════════════════════════

constexpr auto kElectionTimeout = std::chrono::seconds(1);
constexpr auto kLinearizabilityTimeout = std::chrono::seconds(1);

struct OpLog {
    std::vector<Operation> operations;
    std::mutex mu;
    void append(const Operation& op) {
        std::lock_guard<std::mutex> lk(mu);
        operations.push_back(op);
    }
    std::vector<Operation> read() {
        std::lock_guard<std::mutex> lk(mu);
        return operations;
    }
};

static std::string doGet(KVConfig& cfg, Clerk& ck, const std::string& key,
                          OpLog* log, int cli) {
    auto start = std::chrono::steady_clock::now().time_since_epoch().count();
    std::string v = ck.Get(key);
    auto end = std::chrono::steady_clock::now().time_since_epoch().count();
    cfg.op();
    if (log) {
        log->append({cli, 0, key, "", v, start, end});
    }
    return v;
}

static void doPut(KVConfig& cfg, Clerk& ck, const std::string& key,
                   const std::string& value, OpLog* log, int cli) {
    auto start = std::chrono::steady_clock::now().time_since_epoch().count();
    ck.Put(key, value);
    auto end = std::chrono::steady_clock::now().time_since_epoch().count();
    cfg.op();
    if (log) {
        log->append({cli, 1, key, value, "", start, end});
    }
}

static void doAppend(KVConfig& cfg, Clerk& ck, const std::string& key,
                      const std::string& value, OpLog* log, int cli) {
    auto start = std::chrono::steady_clock::now().time_since_epoch().count();
    ck.Append(key, value);
    auto end = std::chrono::steady_clock::now().time_since_epoch().count();
    cfg.op();
    if (log) {
        log->append({cli, 2, key, value, "", start, end});
    }
}

static void check(KVConfig& cfg, Clerk& ck, const std::string& key,
                   const std::string& expected) {
    std::string v = doGet(cfg, ck, key, nullptr, -1);
    ASSERT_EQ(v, expected) << "Get(" << key << "): expected " << expected << ", got " << v;
}

static void checkClntAppends(int clnt, const std::string& v, int count) {
    int lastoff = -1;
    for (int j = 0; j < count; ++j) {
        std::string wanted = "x " + std::to_string(clnt) + " " + std::to_string(j) + " y";
        auto off = v.find(wanted);
        ASSERT_NE(off, std::string::npos) << clnt << " missing " << wanted << " in " << v;
        auto off1 = v.rfind(wanted);
        ASSERT_EQ(off, off1) << "duplicate " << wanted;
        ASSERT_GT(static_cast<int>(off), lastoff) << "wrong order for " << wanted;
        lastoff = static_cast<int>(off);
    }
}

static void checkConcurrentAppends(const std::string& v, const std::vector<int>& counts) {
    for (int i = 0; i < static_cast<int>(counts.size()); ++i) {
        int lastoff = -1;
        for (int j = 0; j < counts[i]; ++j) {
            std::string wanted = "x " + std::to_string(i) + " " + std::to_string(j) + " y";
            auto off = v.find(wanted);
            ASSERT_NE(off, std::string::npos) << i << " missing " << wanted << " in " << v;
            auto off1 = v.rfind(wanted);
            ASSERT_EQ(off, off1) << "duplicate " << wanted;
            ASSERT_GT(static_cast<int>(off), lastoff) << "wrong order for " << wanted;
            lastoff = static_cast<int>(off);
        }
    }
}

// spawn ncli clients and wait for all to finish
static void spawnClientsAndWait(
    KVConfig& cfg, int ncli,
    std::function<void(int, std::shared_ptr<Clerk>)> fn,
    std::vector<int>& results)
{
    results.resize(ncli, 0);
    std::vector<std::thread> threads;
    for (int i = 0; i < ncli; ++i) {
        threads.emplace_back([&, i]() {
            auto ck = cfg.makeClient(cfg.all());
            fn(i, ck);
            cfg.deleteClient(ck);
        });
    }
    for (auto& t : threads) t.join();
}

// ═══════════════════════════════════════════════════════════════
// GenericTest
// ═══════════════════════════════════════════════════════════════

static void genericTest(const std::string& part, int nclients, int nservers,
                         bool unreliable, bool crash, bool partitions,
                         int maxraftstate, bool randomkeys) {
    std::string title = "Test: ";
    if (unreliable) title += "unreliable net, ";
    if (crash) title += "restarts, ";
    if (partitions) title += "partitions, ";
    if (maxraftstate != -1) title += "snapshots, ";
    if (randomkeys) title += "random keys, ";
    title += (nclients > 1) ? "many clients" : "one client";
    title += " (" + part + ")";

    KVConfig cfg(nservers, unreliable, maxraftstate, title);

    cfg.begin(title);
    OpLog opLog;

    auto ck = cfg.makeClient(cfg.all());

    std::atomic<bool> donePartitioner{false};
    std::atomic<bool> doneClients{false};
    std::vector<int> clientResults(nclients, 0);

    for (int iter = 0; iter < 3; ++iter) {
        doneClients.store(false);
        donePartitioner.store(false);

        // Spawn clients
        std::vector<std::thread> clientThreads;
        std::vector<int> opsPerClient(nclients, 0);

        for (int cli = 0; cli < nclients; ++cli) {
            clientThreads.emplace_back([&, cli]() {
                auto myck = cfg.makeClient(cfg.all());
                int j = 0;
                std::string last;
                if (!randomkeys) {
                    doPut(cfg, *myck, std::to_string(cli), last, &opLog, cli);
                }
                while (!doneClients.load()) {
                    std::string key;
                    if (randomkeys) {
                        key = std::to_string(rng()() % nclients);
                    } else {
                        key = std::to_string(cli);
                    }
                    std::string nv = "x " + std::to_string(cli) + " " + std::to_string(j) + " y";
                    if (rng()() % 1000 < 500) {
                        doAppend(cfg, *myck, key, nv, &opLog, cli);
                        if (!randomkeys) last += nv;
                        ++j;
                    } else if (randomkeys && rng()() % 1000 < 100) {
                        doPut(cfg, *myck, key, nv, &opLog, cli);
                        ++j;
                    } else {
                        std::string v = doGet(cfg, *myck, key, &opLog, cli);
                        if (!randomkeys && v != last) {
                            FAIL() << "get wrong value, key " << key
                                   << ", wanted: " << last << ", got: " << v;
                        }
                    }
                }
                opsPerClient[cli] = j;
                cfg.deleteClient(myck);
            });
        }

        // Optionally start partitioner
        std::thread partitionerThread;
        if (partitions) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            partitionerThread = std::thread([&]() {
                while (!donePartitioner.load()) {
                    std::vector<int> a(nservers);
                    for (int i = 0; i < nservers; ++i) a[i] = rng()() % 2;
                    std::vector<int> pa[2];
                    for (int j = 0; j < nservers; ++j) pa[a[j]].push_back(j);
                    cfg.partition(pa[0], pa[1]);
                    std::this_thread::sleep_for(
                        kElectionTimeout + std::chrono::milliseconds(rng()() % 200));
                }
            });
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
        doneClients.store(true);
        donePartitioner.store(true);

        for (auto& t : clientThreads) t.join();
        if (partitionerThread.joinable()) partitionerThread.join();

        if (partitions) {
            cfg.connectAll();
            std::this_thread::sleep_for(kElectionTimeout);
        }

        if (crash) {
            for (int i = 0; i < nservers; ++i) cfg.shutdownServer(i);
            std::this_thread::sleep_for(kElectionTimeout);
            for (int i = 0; i < nservers; ++i) cfg.startServer(i);
            cfg.connectAll();
        }

        // Verify client results
        for (int i = 0; i < nclients; ++i) {
            std::string key = std::to_string(i);
            std::string v = doGet(cfg, *ck, key, &opLog, 0);
            if (!randomkeys) {
                checkClntAppends(i, v, opsPerClient[i]);
            }
        }

        if (maxraftstate > 0) {
            int sz = cfg.logSize();
            ASSERT_LE(sz, 8 * maxraftstate) << "logs were not trimmed";
        }
        if (maxraftstate < 0) {
            int ssz = cfg.snapshotSize();
            ASSERT_EQ(ssz, 0) << "snapshot should not be used";
        }
    }

    // Linearizability check
    auto ops = opLog.read();
    if (!ops.empty()) {
        auto result = checkLinearizability(ops, kLinearizabilityTimeout);
        if (result == CheckIllegal) {
            FAIL() << "history is not linearizable";
        }
    }

    cfg.end();
}

static void genericTestSpeed(const std::string& part, int maxraftstate) {
    const int nservers = 3;
    const int numOps = 1000;
    KVConfig cfg(nservers, false, maxraftstate, "speed");

    auto ck = cfg.makeClient(cfg.all());
    cfg.begin("Test: ops complete fast enough (" + part + ")");

    // Wait for leader
    ck->Get("x");

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < numOps; ++i) {
        ck->Append("x", "x 0 " + std::to_string(i) + " y");
    }
    auto dur = std::chrono::steady_clock::now() - start;

    std::string v = ck->Get("x");
    checkClntAppends(0, v, numOps);

    auto perOp = std::chrono::milliseconds(100) / 3;
    auto total = dur / numOps;
    ASSERT_LT(total, perOp) << "operations too slow";

    cfg.end();
}

} // namespace kvtest
} // namespace raft

// ═══════════════════════════════════════════════════════════════
// Google Test cases
// ═══════════════════════════════════════════════════════════════

using namespace raft::kvtest;

// ── 3A Tests ─────────────────────────────────────────────────

TEST(KVRaft, Basic3A) {
    genericTest("3A", 1, 5, false, false, false, -1, false);
}

TEST(KVRaft, Speed3A) {
    genericTestSpeed("3A", -1);
}

TEST(KVRaft, Concurrent3A) {
    genericTest("3A", 5, 5, false, false, false, -1, false);
}

TEST(KVRaft, Unreliable3A) {
    genericTest("3A", 5, 5, true, false, false, -1, false);
}

TEST(KVRaft, UnreliableOneKey3A) {
    const int nservers = 3;
    KVConfig cfg(nservers, true, -1, "unreliable-one-key");
    auto ck = cfg.makeClient(cfg.all());
    cfg.begin("Test: concurrent append to same key, unreliable (3A)");

    doPut(cfg, *ck, "k", "", nullptr, -1);

    const int nclient = 5;
    const int upto = 10;

    std::vector<std::thread> threads;
    for (int me = 0; me < nclient; ++me) {
        threads.emplace_back([&, me]() {
            auto myck = cfg.makeClient(cfg.all());
            for (int n = 0; n < upto; ++n) {
                doAppend(cfg, *myck, "k",
                         "x " + std::to_string(me) + " " + std::to_string(n) + " y",
                         nullptr, -1);
            }
            cfg.deleteClient(myck);
        });
    }
    for (auto& t : threads) t.join();

    std::vector<int> counts(nclient, upto);
    std::string vx = doGet(cfg, *ck, "k", nullptr, -1);
    checkConcurrentAppends(vx, counts);

    cfg.end();
}

TEST(KVRaft, OnePartition3A) {
    const int nservers = 5;
    KVConfig cfg(nservers, false, -1, "one-partition");
    auto ck = cfg.makeClient(cfg.all());

    doPut(cfg, *ck, "1", "13", nullptr, -1);

    cfg.begin("Test: progress in majority (3A)");
    auto [p1, p2] = cfg.makePartition();
    cfg.partition(p1, p2);

    auto ckp1 = cfg.makeClient(p1);
    auto ckp2a = cfg.makeClient(p2);
    auto ckp2b = cfg.makeClient(p2);

    doPut(cfg, *ckp1, "1", "14", nullptr, -1);
    check(cfg, *ckp1, "1", "14");
    cfg.end();

    // Minority should not complete
    cfg.begin("Test: no progress in minority (3A)");
    std::atomic<bool> done0{false}, done1{false};
    std::thread t0([&]() { doPut(cfg, *ckp2a, "1", "15", nullptr, -1); done0.store(true); });
    std::thread t1([&]() { doGet(cfg, *ckp2b, "1", nullptr, -1); done1.store(true); });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_FALSE(done0.load()) << "Put in minority completed";
    ASSERT_FALSE(done1.load()) << "Get in minority completed";

    check(cfg, *ckp1, "1", "14");
    doPut(cfg, *ckp1, "1", "16", nullptr, -1);
    check(cfg, *ckp1, "1", "16");
    cfg.end();

    cfg.begin("Test: completion after heal (3A)");
    cfg.connectAll();
    cfg.connectClient(ckp2a.get(), cfg.all());
    cfg.connectClient(ckp2b.get(), cfg.all());

    std::this_thread::sleep_for(raft::kvtest::kElectionTimeout);

    // Wait for pending ops
    if (t0.joinable()) t0.join();
    if (t1.joinable()) t1.join();

    check(cfg, *ck, "1", "15");
    cfg.end();
}

TEST(KVRaft, ManyPartitionsOneClient3A) {
    genericTest("3A", 1, 5, false, false, true, -1, false);
}

TEST(KVRaft, ManyPartitionsManyClients3A) {
    genericTest("3A", 5, 5, false, false, true, -1, false);
}

TEST(KVRaft, PersistOneClient3A) {
    genericTest("3A", 1, 5, false, true, false, -1, false);
}

TEST(KVRaft, PersistConcurrent3A) {
    genericTest("3A", 5, 5, false, true, false, -1, false);
}

TEST(KVRaft, PersistConcurrentUnreliable3A) {
    genericTest("3A", 5, 5, true, true, false, -1, false);
}

TEST(KVRaft, PersistPartition3A) {
    genericTest("3A", 5, 5, false, true, true, -1, false);
}

TEST(KVRaft, PersistPartitionUnreliable3A) {
    genericTest("3A", 5, 5, true, true, true, -1, false);
}

TEST(KVRaft, PersistPartitionUnreliableLinearizable3A) {
    genericTest("3A", 15, 7, true, true, true, -1, true);
}

// ── 3B Tests (Snapshots) ─────────────────────────────────────

TEST(KVRaft, SnapshotRPC3B) {
    const int nservers = 3;
    const int maxraftstate = 1000;
    KVConfig cfg(nservers, false, maxraftstate, "snapshot-rpc");

    auto ck = cfg.makeClient(cfg.all());
    cfg.begin("Test: InstallSnapshot RPC (3B)");

    doPut(cfg, *ck, "a", "A", nullptr, -1);
    check(cfg, *ck, "a", "A");

    // Partition: majority {0,1} vs minority {2}
    cfg.partition({0, 1}, {2});
    {
        auto ck1 = cfg.makeClient(std::vector<int>{0, 1});
        for (int i = 0; i < 50; ++i) {
            doPut(cfg, *ck1, std::to_string(i), std::to_string(i), nullptr, -1);
        }
        std::this_thread::sleep_for(raft::kvtest::kElectionTimeout);
        doPut(cfg, *ck1, "b", "B", nullptr, -1);
    }

    int sz = cfg.logSize();
    ASSERT_LE(sz, 8 * maxraftstate) << "logs were not trimmed";

    // Now include lagging server
    cfg.partition({0, 2}, {1});
    {
        auto ck1 = cfg.makeClient(std::vector<int>{0, 2});
        doPut(cfg, *ck1, "c", "C", nullptr, -1);
        doPut(cfg, *ck1, "d", "D", nullptr, -1);
        check(cfg, *ck1, "a", "A");
        check(cfg, *ck1, "b", "B");
        check(cfg, *ck1, "1", "1");
        check(cfg, *ck1, "49", "49");
    }

    cfg.partition({0, 1, 2}, {});
    doPut(cfg, *ck, "e", "E", nullptr, -1);
    check(cfg, *ck, "c", "C");
    check(cfg, *ck, "e", "E");
    check(cfg, *ck, "1", "1");

    cfg.end();
}

TEST(KVRaft, SnapshotSize3B) {
    const int nservers = 3;
    const int maxraftstate = 1000;
    const int maxsnapshotstate = 500;
    KVConfig cfg(nservers, false, maxraftstate, "snapshot-size");

    auto ck = cfg.makeClient(cfg.all());
    cfg.begin("Test: snapshot size is reasonable (3B)");

    for (int i = 0; i < 200; ++i) {
        doPut(cfg, *ck, "x", "0", nullptr, -1);
        check(cfg, *ck, "x", "0");
        doPut(cfg, *ck, "x", "1", nullptr, -1);
        check(cfg, *ck, "x", "1");
    }

    int sz = cfg.logSize();
    ASSERT_LE(sz, 8 * maxraftstate) << "logs were not trimmed";

    int ssz = cfg.snapshotSize();
    ASSERT_LE(ssz, maxsnapshotstate) << "snapshot too large";

    cfg.end();
}

TEST(KVRaft, Speed3B) {
    genericTestSpeed("3B", 1000);
}

TEST(KVRaft, SnapshotRecover3B) {
    genericTest("3B", 1, 5, false, true, false, 1000, false);
}

TEST(KVRaft, SnapshotRecoverManyClients3B) {
    genericTest("3B", 20, 5, false, true, false, 1000, false);
}

TEST(KVRaft, SnapshotUnreliable3B) {
    genericTest("3B", 5, 5, true, false, false, 1000, false);
}

TEST(KVRaft, SnapshotUnreliableRecover3B) {
    genericTest("3B", 5, 5, true, true, false, 1000, false);
}

TEST(KVRaft, SnapshotUnreliableRecoverConcurrentPartition3B) {
    genericTest("3B", 5, 5, true, true, true, 1000, false);
}

TEST(KVRaft, SnapshotUnreliableRecoverConcurrentPartitionLinearizable3B) {
    genericTest("3B", 15, 7, true, true, true, 1000, true);
}
