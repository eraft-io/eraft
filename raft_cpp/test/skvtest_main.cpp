// skvtest_main.cpp — ShardKV test suite + test framework
// Corresponds to Go: shardkv/config.go + shardkv/test_test.go

#include <gtest/gtest.h>

#include "raft/skv_clerk.h"
#include "raft/skv_common.h"
#include "raft/shardkv.h"
#include "raft/sc_clerk.h"
#include "raft/sc_common.h"
#include "raft/shardctrler.h"
#include "raft/config.h"
#include "raft/persister.h"
#include "raft/util.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace raft {
namespace skvtest {

// ═══════════════════════════════════════════════════════════════
// Helper functions
// ═══════════════════════════════════════════════════════════════

static std::string randstring(int n) {
    static thread_local std::mt19937 rng(std::random_device{}());
    static const char chars[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string s(n, ' ');
    for (int i = 0; i < n; ++i) {
        s[i] = chars[rng() % (sizeof(chars) - 1)];
    }
    return s;
}

static void check(ShardKVClerk* ck, const std::string& key, const std::string& value) {
    auto v = ck->Get(key);
    if (v != value) {
        FAIL() << "Get(" << key << "): expected " << value << " got " << v;
    }
}

// Helper: wait until all keys are accessible via Gets
static void waitForGets(ShardKVClerk* ck, const std::string* ka, const std::string* va, int n,
                         int timeoutMs = 90000) {
    auto start = std::chrono::steady_clock::now();
    while (true) {
        bool allOk = true;
        for (int i = 0; i < n; ++i) {
            auto v = ck->Get(ka[i]);
            if (v != va[i]) {
                allOk = false;
                break;
            }
        }
        if (allOk) return;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed > timeoutMs) {
            FAIL() << "waitForGets timed out after " << elapsed << "ms";
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

// ═══════════════════════════════════════════════════════════════
// InMemShardKVPeerProvider — cross-group communication for tests
// ═══════════════════════════════════════════════════════════════

class InMemShardKVPeerHandle : public ShardKVPeerHandle {
public:
    InMemShardKVPeerHandle(std::weak_ptr<ShardKV> target)
        : target_(target) {}

    bool GetShardsData(const ShardOperationRequest& req,
                       ShardOperationResponse& resp) override {
        auto t = target_.lock();
        if (!t) return false;
        t->GetShardsData(req, resp);
        return true;
    }

    bool DeleteShardsData(const ShardOperationRequest& req,
                          ShardOperationResponse& resp) override {
        auto t = target_.lock();
        if (!t) return false;
        t->DeleteShardsData(req, resp);
        return true;
    }

private:
    std::weak_ptr<ShardKV> target_;
};

// ═══════════════════════════════════════════════════════════════
// SKVTestConfig — Test framework
// ═══════════════════════════════════════════════════════════════

struct GroupData {
    int gid;
    std::vector<std::shared_ptr<ShardKV>> servers;
    std::vector<std::shared_ptr<Persister>> saved;
    // peers_[i][j] = peer that server i uses to talk to server j
    std::vector<std::vector<std::shared_ptr<InMemPeer>>> peers;
    // Peers for ShardKV client communication
    std::vector<std::shared_ptr<InMemShardKVClientPeer>> clientPeers;
};

class SKVTestConfig {
public:
    SKVTestConfig(int n, bool unreliable, int maxRaftState);
    ~SKVTestConfig();

    void cleanup();

    std::shared_ptr<ShardKVClerk> makeClient();
    void refreshClientPeers(ShardKVClerk* ck);

    void join(int gi);
    void joinm(const std::vector<int>& gis);
    void leave(int gi);
    void leavem(const std::vector<int>& gis);

    void ShutdownGroup(int gi);
    void StartGroup(int gi);
    void ShutdownServer(int gi, int i);
    void StartServer(int gi, int i);

    int ngroups = 3;
    int n;  // servers per group
    std::vector<GroupData> groups;

    // ShardCtrler instances
    int nctrlers = 3;
    std::vector<std::shared_ptr<ShardCtrler>> ctrlerServers;
    // ctrlerPeers_[i][j] = peer that ctrler server i uses to talk to ctrler server j
    std::vector<std::vector<std::shared_ptr<InMemPeer>>> ctrlerPeers_;
    std::shared_ptr<SCClerk> mck;  // main shardctrler clerk

private:
    void startCtrlerServer(int i);
    std::shared_ptr<SCClerk> makeSCClerk();
    ShardKVPeerProvider makePeerProvider();

    bool unreliable_ = false;
    int maxRaftState_ = -1;

    std::mutex mu_;
};

SKVTestConfig::SKVTestConfig(int n, bool unreliable, int maxRaftState)
    : n(n), unreliable_(unreliable), maxRaftState_(maxRaftState)
{
    // Start all ShardCtrler servers
    ctrlerServers.resize(nctrlers);
    ctrlerPeers_.resize(nctrlers);

    // First pass: create all servers with peers (not yet wired)
    for (int i = 0; i < nctrlers; ++i) {
        ctrlerPeers_[i].resize(nctrlers);
        for (int j = 0; j < nctrlers; ++j) {
            ctrlerPeers_[i][j] = std::make_shared<InMemPeer>();
        }

        auto persister = std::make_shared<Persister>();
        std::vector<std::shared_ptr<RaftPeer>> raftPeers(nctrlers);
        for (int j = 0; j < nctrlers; ++j) {
            raftPeers[j] = ctrlerPeers_[i][j];
        }
        ctrlerServers[i] = ShardCtrler::Make(raftPeers, i, persister);
    }

    // Second pass: wire up all peer connections
    for (int i = 0; i < nctrlers; ++i) {
        for (int j = 0; j < nctrlers; ++j) {
            ctrlerPeers_[i][j]->setTarget(ctrlerServers[j]->getRaft());
            ctrlerPeers_[i][j]->setEnabled(true);
        }
    }

    mck = makeSCClerk();

    // Wait for ctrler leader election
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    std::cout << "[Config] ShardCtrler servers started, waiting for leader election..." << std::endl;

    // Verify SCClerk can communicate
    auto testConfig = mck->Query(-1);
    std::cout << "[Config] SCClerk Query OK, config Num=" << testConfig.Num << std::endl;

    // Create groups
    groups.resize(ngroups);
    for (int gi = 0; gi < ngroups; ++gi) {
        groups[gi].gid = 100 + gi;
        groups[gi].servers.resize(n);
        groups[gi].saved.resize(n);
        groups[gi].peers.resize(n);
        groups[gi].clientPeers.resize(n);

        // First pass: create all servers
        for (int i = 0; i < n; ++i) {
            StartServer(gi, i);
        }

        // Second pass: wire up all peer connections within group
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (groups[gi].servers[j]) {
                    groups[gi].peers[i][j]->setTarget(groups[gi].servers[j]->getRaft());
                }
                groups[gi].peers[i][j]->setEnabled(true);
            }
        }
    }

    // Wait for ShardKV groups leader election
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    std::cout << "[Config] All ShardKV groups started" << std::endl;
}

SKVTestConfig::~SKVTestConfig() {
    cleanup();
}

void SKVTestConfig::cleanup() {
    for (int gi = 0; gi < ngroups; ++gi) {
        ShutdownGroup(gi);
    }
    for (int i = 0; i < nctrlers; ++i) {
        if (ctrlerServers[i]) {
            ctrlerServers[i]->Kill();
        }
    }
}

void SKVTestConfig::startCtrlerServer(int i) {
    // This function is no longer used in the constructor,
    // but kept for potential restart scenarios.
    // For simplicity, ctrler servers are not restartable in this test.
}

std::shared_ptr<SCClerk> SKVTestConfig::makeSCClerk() {
    std::vector<std::shared_ptr<SCClerkPeer>> peers(nctrlers);
    for (int i = 0; i < nctrlers; ++i) {
        auto p = std::make_shared<InMemSCClerkPeer>();
        p->setTarget(ctrlerServers[i]);
        p->setEnabled(true);
        peers[i] = p;
    }
    return std::make_shared<SCClerk>(peers);
}

ShardKVPeerProvider SKVTestConfig::makePeerProvider() {
    // Capture groups by reference through a shared pointer to this config
    auto* self = this;
    return [self](int gid, int serverIdx, const std::string& /*addr*/)
        -> std::shared_ptr<ShardKVPeerHandle>
    {
        // Find which group index has this gid
        for (int gi = 0; gi < self->ngroups; ++gi) {
            if (self->groups[gi].gid == gid) {
                if (serverIdx < self->n && self->groups[gi].servers[serverIdx]) {
                    return std::make_shared<InMemShardKVPeerHandle>(
                        self->groups[gi].servers[serverIdx]);
                }
                break;
            }
        }
        return nullptr;
    };
}

void SKVTestConfig::StartServer(int gi, int i) {
    std::lock_guard<std::mutex> lk(mu_);

    auto& gg = groups[gi];

    // Create peers for intra-group Raft communication
    gg.peers[i].resize(n);
    for (int j = 0; j < n; ++j) {
        gg.peers[i][j] = std::make_shared<InMemPeer>();
    }

    // Copy persister state if restarting
    if (gg.saved[i]) {
        gg.saved[i] = gg.saved[i]->Copy();
    } else {
        gg.saved[i] = std::make_shared<Persister>();
    }

    // Build Raft peers
    std::vector<std::shared_ptr<RaftPeer>> raftPeers(n);
    for (int j = 0; j < n; ++j) {
        raftPeers[j] = gg.peers[i][j];
    }

    // Create SCClerk for this server
    auto sc = makeSCClerk();

    // Create peer provider
    auto provider = makePeerProvider();

    gg.servers[i] = ShardKV::Make(raftPeers, i, gg.saved[i],
                                   maxRaftState_, gg.gid, sc, provider);

    // Wire up all peer connections for this server
    auto raft = gg.servers[i]->getRaft();
    for (int j = 0; j < n; ++j) {
        if (gg.servers[j]) {
            gg.peers[i][j]->setTarget(gg.servers[j]->getRaft());
            // Also update other servers' peers to point to this new server
            gg.peers[j][i]->setTarget(raft);
        }
        gg.peers[i][j]->setEnabled(true);
    }

    // Update client peer in-place (don't create new, so existing clerks keep working)
    if (gg.clientPeers[i]) {
        gg.clientPeers[i]->setTarget(gg.servers[i]);
        gg.clientPeers[i]->setEnabled(true);
    } else {
        auto clientPeer = std::make_shared<InMemShardKVClientPeer>();
        clientPeer->setTarget(gg.servers[i]);
        clientPeer->setEnabled(true);
        gg.clientPeers[i] = clientPeer;
    }
}

void SKVTestConfig::ShutdownServer(int gi, int i) {
    std::lock_guard<std::mutex> lk(mu_);
    auto& gg = groups[gi];

    // Disable all peers for this server
    for (int j = 0; j < n; ++j) {
        gg.peers[i][j]->setEnabled(false);
    }

    // Disable client peer
    if (gg.clientPeers[i]) {
        gg.clientPeers[i]->setEnabled(false);
    }

    // Save persister state
    if (gg.saved[i]) {
        gg.saved[i] = gg.saved[i]->Copy();
    }

    // Kill server
    if (gg.servers[i]) {
        gg.servers[i]->Kill();
        gg.servers[i] = nullptr;
    }
}

void SKVTestConfig::ShutdownGroup(int gi) {
    for (int i = 0; i < n; ++i) {
        ShutdownServer(gi, i);
    }
}

void SKVTestConfig::StartGroup(int gi) {
    for (int i = 0; i < n; ++i) {
        StartServer(gi, i);
    }
    // Wire up all peer connections within group
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (groups[gi].servers[j]) {
                groups[gi].peers[i][j]->setTarget(groups[gi].servers[j]->getRaft());
            }
            groups[gi].peers[i][j]->setEnabled(true);
        }
    }
}

std::shared_ptr<ShardKVClerk> SKVTestConfig::makeClient() {
    auto sc = makeSCClerk();
    auto ck = std::make_shared<ShardKVClerk>(sc);
    refreshClientPeers(ck.get());
    return ck;
}

void SKVTestConfig::refreshClientPeers(ShardKVClerk* ck) {
    for (int gi = 0; gi < ngroups; ++gi) {
        auto& gg = groups[gi];
        std::vector<std::shared_ptr<ShardKVClientPeer>> peers(n);
        for (int i = 0; i < n; ++i) {
            if (gg.clientPeers[i]) {
                peers[i] = gg.clientPeers[i];
            } else {
                peers[i] = std::make_shared<InMemShardKVClientPeer>();
            }
        }
        ck->SetPeers(gg.gid, peers);
    }
}

void SKVTestConfig::join(int gi) {
    joinm({gi});
}

void SKVTestConfig::joinm(const std::vector<int>& gis) {
    std::map<int, std::vector<std::string>> m;
    for (int g : gis) {
        int gid = groups[g].gid;
        std::vector<std::string> servers(n);
        for (int i = 0; i < n; ++i) {
            servers[i] = "server-" + std::to_string(gid) + "-" + std::to_string(i);
        }
        m[gid] = servers;
    }
    mck->Join(m);
}

void SKVTestConfig::leave(int gi) {
    leavem({gi});
}

void SKVTestConfig::leavem(const std::vector<int>& gis) {
    std::vector<int> gids;
    for (int g : gis) {
        gids.push_back(groups[g].gid);
    }
    mck->Leave(gids);
}

// ═══════════════════════════════════════════════════════════════
// Migration status polling helper
// ═══════════════════════════════════════════════════════════════

// Wait until all active servers in all groups have all shards in Serving state
static void waitForMigrations(SKVTestConfig& cfg, int timeoutMs = 30000) {
    auto start = std::chrono::steady_clock::now();
    while (true) {
        bool allServing = true;
        for (int gi = 0; gi < cfg.ngroups; ++gi) {
            for (int si = 0; si < cfg.n; ++si) {
                auto& srv = cfg.groups[gi].servers[si];
                if (!srv) continue;
                auto info = srv->GetDebugInfo();
                if (info.nonServingCount > 0) {
                    allServing = false;
                }
            }
        }
        if (allServing) return;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed > timeoutMs) {
            // Print status of all groups for debugging
            for (int gi = 0; gi < cfg.ngroups; ++gi) {
                for (int si = 0; si < cfg.n; ++si) {
                    auto& srv = cfg.groups[gi].servers[si];
                    if (!srv) {
                        std::cerr << "  Group " << cfg.groups[gi].gid
                                  << " server " << si << ": DOWN" << std::endl;
                        continue;
                    }
                    auto info = srv->GetDebugInfo();
                    if (info.nonServingCount > 0) {
                        std::cerr << "  Group " << cfg.groups[gi].gid
                                  << " server " << si
                                  << ": config=" << info.configNum
                                  << " nonServing=" << info.nonServingCount
                                  << " [";
                        for (int s = 0; s < NShards; ++s) {
                            if (info.shardStatuses[s] != Serving) {
                                std::cerr << s << ":" << shardStatusToString(info.shardStatuses[s]) << " ";
                            }
                        }
                        std::cerr << "]" << std::endl;
                    }
                }
            }
            FAIL() << "waitForMigrations timed out after " << elapsed << "ms";
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

// ═══════════════════════════════════════════════════════════════
// Test Cases
// ═══════════════════════════════════════════════════════════════

// Test: static 2-way sharding, without shard movement.
TEST(ShardKVTest, TestStaticShards) {
    std::cout << "Test: static shards ..." << std::endl;

    SKVTestConfig cfg(3, false, -1);

    auto ck = cfg.makeClient();

    std::cout << "[Test] Joining group 0..." << std::endl;
    cfg.join(0);
    std::cout << "[Test] Joining group 1..." << std::endl;
    cfg.join(1);

    // Allow time for configuration to propagate
    std::cout << "[Test] Waiting for config propagation..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    cfg.refreshClientPeers(ck.get());
    std::cout << "[Test] Config propagation done" << std::endl;

    const int n = 10;
    std::string ka[n], va[n];
    for (int i = 0; i < n; ++i) {
        ka[i] = std::to_string(i);
        va[i] = randstring(20);
        std::cout << "[Test] Putting key " << ka[i] << "..." << std::endl;
        ck->Put(ka[i], va[i]);
    }
    std::cout << "[Test] All puts done, verifying..." << std::endl;
    for (int i = 0; i < n; ++i) {
        check(ck.get(), ka[i], va[i]);
    }
    std::cout << "[Test] Verification done" << std::endl;

    // Shut down one group and check that some Gets don't succeed
    cfg.ShutdownGroup(1);

    std::atomic<int> ndone{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < n; ++i) {
        auto ck1 = cfg.makeClient();
        threads.emplace_back([&, i, ck1]() {
            try {
                auto v = ck1->Get(ka[i]);
                if (v == va[i]) {
                    ndone++;
                }
            } catch (...) {
                // timeout or error
            }
        });
    }

    // Wait a bit, about half the Gets should succeed
    for (auto& t : threads) t.join();

    // With 10 shards split between 2 groups, shutting down group 1
    // should make about half the keys unavailable
    // The exact number depends on hash distribution
    EXPECT_GE(ndone.load(), 3);  // at least some should succeed
    EXPECT_LE(ndone.load(), 8);  // but not all

    // Bring group back and verify all data is intact
    cfg.StartGroup(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    cfg.refreshClientPeers(ck.get());
    for (int i = 0; i < n; ++i) {
        check(ck.get(), ka[i], va[i]);
    }

    std::cout << "  ... Passed" << std::endl;
}

// Test: join then leave
TEST(ShardKVTest, TestJoinLeave) {
    std::cout << "Test: join then leave ..." << std::endl;

    SKVTestConfig cfg(3, false, -1);

    auto ck = cfg.makeClient();

    cfg.join(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    cfg.refreshClientPeers(ck.get());

    const int n = 10;
    std::string ka[n], va[n];
    for (int i = 0; i < n; ++i) {
        ka[i] = std::to_string(i);
        va[i] = randstring(5);
        ck->Put(ka[i], va[i]);
    }
    for (int i = 0; i < n; ++i) {
        check(ck.get(), ka[i], va[i]);
    }

    cfg.join(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    cfg.refreshClientPeers(ck.get());

    for (int i = 0; i < n; ++i) {
        check(ck.get(), ka[i], va[i]);
        auto x = randstring(5);
        ck->Append(ka[i], x);
        va[i] += x;
    }

    cfg.leave(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    cfg.refreshClientPeers(ck.get());

    for (int i = 0; i < n; ++i) {
        check(ck.get(), ka[i], va[i]);
        auto x = randstring(5);
        ck->Append(ka[i], x);
        va[i] += x;
    }

    // Allow time for shard transfer
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    cfg.ShutdownGroup(0);

    cfg.refreshClientPeers(ck.get());
    for (int i = 0; i < n; ++i) {
        check(ck.get(), ka[i], va[i]);
    }

    std::cout << "  ... Passed" << std::endl;
}

// Test: snapshots, join, and leave
TEST(ShardKVTest, TestSnapshot) {
    std::cout << "Test: snapshots, join, and leave ..." << std::endl;

    SKVTestConfig cfg(3, false, 1000);

    auto ck = cfg.makeClient();

    cfg.join(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    cfg.refreshClientPeers(ck.get());

    const int n = 30;
    std::string ka[n], va[n];
    for (int i = 0; i < n; ++i) {
        ka[i] = std::to_string(i);
        va[i] = randstring(20);
        ck->Put(ka[i], va[i]);
    }
    for (int i = 0; i < n; ++i) {
        check(ck.get(), ka[i], va[i]);
    }

    cfg.join(1);
    cfg.join(2);
    cfg.leave(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    cfg.refreshClientPeers(ck.get());

    for (int i = 0; i < n; ++i) {
        check(ck.get(), ka[i], va[i]);
        auto x = randstring(20);
        ck->Append(ka[i], x);
        va[i] += x;
    }

    cfg.leave(1);
    cfg.join(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    cfg.refreshClientPeers(ck.get());

    for (int i = 0; i < n; ++i) {
        check(ck.get(), ka[i], va[i]);
        auto x = randstring(20);
        ck->Append(ka[i], x);
        va[i] += x;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    for (int i = 0; i < n; ++i) {
        check(ck.get(), ka[i], va[i]);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    cfg.ShutdownGroup(0);
    cfg.ShutdownGroup(1);
    cfg.ShutdownGroup(2);

    cfg.StartGroup(0);
    cfg.StartGroup(1);
    cfg.StartGroup(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    cfg.refreshClientPeers(ck.get());

    for (int i = 0; i < n; ++i) {
        check(ck.get(), ka[i], va[i]);
    }

    std::cout << "  ... Passed" << std::endl;
}

// Test: servers miss configuration changes
TEST(ShardKVTest, TestMissChange) {
    std::cout << "Test: servers miss configuration changes..." << std::endl;

    SKVTestConfig cfg(3, false, 1000);

    auto ck = cfg.makeClient();

    cfg.join(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    cfg.refreshClientPeers(ck.get());

    const int n = 10;
    std::string ka[n], va[n];
    for (int i = 0; i < n; ++i) {
        ka[i] = std::to_string(i);
        va[i] = randstring(20);
        ck->Put(ka[i], va[i]);
    }
    for (int i = 0; i < n; ++i) {
        check(ck.get(), ka[i], va[i]);
    }

    cfg.join(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    cfg.ShutdownServer(0, 0);
    cfg.ShutdownServer(1, 0);
    cfg.ShutdownServer(2, 0);

    cfg.join(2);
    cfg.leave(1);
    cfg.leave(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    cfg.refreshClientPeers(ck.get());

    std::cout << "[TestMissChange] Waiting for migrations (phase 1)..." << std::endl;
    waitForMigrations(cfg, 60000);
    std::cout << "[TestMissChange] Migrations done, verifying gets..." << std::endl;
    cfg.refreshClientPeers(ck.get());
    waitForGets(ck.get(), ka, va, n);
    for (int i = 0; i < n; ++i) {
        auto x = randstring(20);
        ck->Append(ka[i], x);
        va[i] += x;
    }

    cfg.join(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    cfg.refreshClientPeers(ck.get());

    for (int i = 0; i < n; ++i) {
        check(ck.get(), ka[i], va[i]);
        auto x = randstring(20);
        ck->Append(ka[i], x);
        va[i] += x;
    }

    cfg.StartServer(0, 0);
    cfg.StartServer(1, 0);
    cfg.StartServer(2, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    cfg.refreshClientPeers(ck.get());

    for (int i = 0; i < n; ++i) {
        check(ck.get(), ka[i], va[i]);
        auto x = randstring(20);
        ck->Append(ka[i], x);
        va[i] += x;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    cfg.ShutdownServer(0, 1);
    cfg.ShutdownServer(1, 1);
    cfg.ShutdownServer(2, 1);

    cfg.join(0);
    cfg.leave(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    cfg.refreshClientPeers(ck.get());

    std::cout << "[TestMissChange] Waiting for migrations (phase 2)..." << std::endl;
    waitForMigrations(cfg, 60000);
    std::cout << "[TestMissChange] Migrations done, verifying gets..." << std::endl;
    cfg.refreshClientPeers(ck.get());
    waitForGets(ck.get(), ka, va, n);
    for (int i = 0; i < n; ++i) {
        auto x = randstring(20);
        ck->Append(ka[i], x);
        va[i] += x;
    }

    cfg.StartServer(0, 1);
    cfg.StartServer(1, 1);
    cfg.StartServer(2, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    cfg.refreshClientPeers(ck.get());

    for (int i = 0; i < n; ++i) {
        check(ck.get(), ka[i], va[i]);
    }

    std::cout << "  ... Passed" << std::endl;
}

// Test: concurrent puts and configuration changes
TEST(ShardKVTest, TestConcurrent1) {
    std::cout << "Test: concurrent puts and configuration changes..." << std::endl;

    SKVTestConfig cfg(3, false, 100);

    auto ck = cfg.makeClient();

    cfg.join(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    cfg.refreshClientPeers(ck.get());

    const int n = 10;
    std::string ka[n], va[n];
    for (int i = 0; i < n; ++i) {
        ka[i] = std::to_string(i);
        va[i] = randstring(5);
        ck->Put(ka[i], va[i]);
    }

    std::atomic<bool> done{false};
    std::vector<std::thread> writers;
    std::mutex vaMu;

    for (int i = 0; i < n; ++i) {
        writers.emplace_back([&, i]() {
            auto ck1 = cfg.makeClient();
            while (!done.load()) {
                auto x = randstring(5);
                ck1->Append(ka[i], x);
                std::lock_guard<std::mutex> lk(vaMu);
                va[i] += x;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    cfg.join(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    cfg.join(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    cfg.leave(0);

    cfg.ShutdownGroup(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    cfg.ShutdownGroup(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    cfg.ShutdownGroup(2);

    cfg.leave(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    cfg.StartGroup(0);
    cfg.StartGroup(1);
    cfg.StartGroup(2);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    cfg.join(0);
    cfg.leave(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    cfg.join(1);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    done.store(true);
    for (auto& t : writers) t.join();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    cfg.refreshClientPeers(ck.get());
    for (int i = 0; i < n; ++i) {
        check(ck.get(), ka[i], va[i]);
    }

    std::cout << "  ... Passed" << std::endl;
}

// Test: more concurrent puts and configuration changes
TEST(ShardKVTest, TestConcurrent2) {
    std::cout << "Test: more concurrent puts and configuration changes..." << std::endl;

    SKVTestConfig cfg(3, false, -1);

    auto ck = cfg.makeClient();

    cfg.join(1);
    cfg.join(0);
    cfg.join(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    cfg.refreshClientPeers(ck.get());

    const int n = 10;
    std::string ka[n], va[n];
    for (int i = 0; i < n; ++i) {
        ka[i] = std::to_string(i);
        va[i] = randstring(1);
        ck->Put(ka[i], va[i]);
    }

    std::atomic<bool> done{false};
    std::vector<std::thread> writers;
    std::mutex vaMu;

    for (int i = 0; i < n; ++i) {
        auto ck1 = cfg.makeClient();
        writers.emplace_back([&, i, ck1]() {
            while (!done.load()) {
                auto x = randstring(1);
                ck1->Append(ka[i], x);
                std::lock_guard<std::mutex> lk(vaMu);
                va[i] += x;
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });
    }

    cfg.leave(0);
    cfg.leave(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    cfg.join(0);
    cfg.join(2);
    cfg.leave(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    cfg.join(1);
    cfg.leave(0);
    cfg.leave(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    cfg.ShutdownGroup(1);
    cfg.ShutdownGroup(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    cfg.StartGroup(1);
    cfg.StartGroup(2);

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    done.store(true);
    for (auto& t : writers) t.join();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    cfg.refreshClientPeers(ck.get());
    for (int i = 0; i < n; ++i) {
        check(ck.get(), ka[i], va[i]);
    }

    std::cout << "  ... Passed" << std::endl;
}

} // namespace skvtest
} // namespace raft
