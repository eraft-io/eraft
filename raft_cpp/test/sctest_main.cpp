// sctest_main.cpp — ShardCtrler test suite + SCTestConfig framework
// Corresponds to Go: shardctrler/config.go + shardctrler/test_test.go

#include <gtest/gtest.h>

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
#include <numeric>
#include <string>
#include <thread>
#include <vector>

namespace raft {
namespace sctest {

// ═══════════════════════════════════════════════════════════════
// Helper functions
// ═══════════════════════════════════════════════════════════════

static constexpr auto kElectionTimeout = std::chrono::milliseconds(1000);

static void check(const SCConfig& c, const std::vector<int>& groups,
                  const std::string& testName) {
    if (static_cast<int>(c.Groups.size()) != static_cast<int>(groups.size())) {
        FAIL() << testName << ": wanted " << groups.size()
               << " groups, got " << c.Groups.size();
    }

    // are the groups as expected?
    for (int g : groups) {
        if (c.Groups.find(g) == c.Groups.end()) {
            FAIL() << testName << ": missing group " << g;
        }
    }

    // any un-allocated shards?
    if (!groups.empty()) {
        for (int s = 0; s < NShards; ++s) {
            int gid = c.Shards[s];
            if (c.Groups.find(gid) == c.Groups.end()) {
                FAIL() << testName << ": shard " << s
                       << " -> invalid group " << gid;
            }
        }
    }

    // more or less balanced sharding?
    std::map<int, int> counts;
    for (int s = 0; s < NShards; ++s) {
        counts[c.Shards[s]]++;
    }
    int min = 257, max = 0;
    for (auto& [gid, _] : c.Groups) {
        if (counts[gid] > max) max = counts[gid];
        if (counts[gid] < min) min = counts[gid];
    }
    if (max > min + 1) {
        FAIL() << testName << ": max " << max
               << " too much larger than min " << min;
    }
}

static void check_same_config(const SCConfig& c1, const SCConfig& c2,
                              const std::string& testName) {
    ASSERT_EQ(c1.Num, c2.Num) << testName << ": Num wrong";
    ASSERT_EQ(c1.Shards, c2.Shards) << testName << ": Shards wrong";
    ASSERT_EQ(c1.Groups.size(), c2.Groups.size())
        << testName << ": number of Groups is wrong";
    for (auto& [gid, sa] : c1.Groups) {
        auto it = c2.Groups.find(gid);
        ASSERT_NE(it, c2.Groups.end()) << testName << ": Groups wrong";
        ASSERT_EQ(it->second.size(), sa.size())
            << testName << ": len(Groups) wrong";
        for (size_t j = 0; j < sa.size(); ++j) {
            ASSERT_EQ(sa[j], it->second[j]) << testName << ": Groups wrong";
        }
    }
}

// ═══════════════════════════════════════════════════════════════
// SCTestConfig — Test framework
// ═══════════════════════════════════════════════════════════════

class SCTestConfig {
public:
    SCTestConfig(int n, bool unreliable, const std::string& testName);
    ~SCTestConfig();

    std::shared_ptr<SCClerk> makeClient(const std::vector<int>& to);
    void connectClient(SCClerk* ck, const std::vector<int>& to);
    void disconnectClient(SCClerk* ck, const std::vector<int>& from);
    void deleteClient(std::shared_ptr<SCClerk> ck);

    void connect(int i);
    void disconnect(int i);
    void connectAll();

    void shutdownServer(int i);
    void startServer(int i);

    std::pair<bool, int> Leader();

    int n() const { return n_; }

    std::vector<int> all() const {
        std::vector<int> a(n_);
        std::iota(a.begin(), a.end(), 0);
        return a;
    }

    void begin(const std::string& desc) {
        std::cout << desc << " ..." << std::endl;
    }

    void end() {
        std::cout << "  ... Passed" << std::endl;
    }

private:
    void cleanup();

    int n_;
    bool unreliable_;
    std::string testName_;

    // Per-server state
    std::vector<std::shared_ptr<ShardCtrler>> servers_;
    std::vector<std::shared_ptr<Persister>> saved_;
    std::vector<std::vector<std::shared_ptr<InMemPeer>>> raftPeers_;
    std::vector<bool> connected_;

    // Per-clerk SC peers: clerkPeers_[ck][j]
    std::mutex mu_;
    std::map<SCClerk*, std::vector<std::shared_ptr<InMemSCClerkPeer>>> clerkPeers_;
};

SCTestConfig::SCTestConfig(int n, bool unreliable, const std::string& testName)
    : n_(n), unreliable_(unreliable), testName_(testName)
{
    servers_.resize(n);
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
        // InMemPeer already has enabled/disabled for Raft
    }
}

SCTestConfig::~SCTestConfig() {
    cleanup();
}

void SCTestConfig::cleanup() {
    for (int i = 0; i < n_; ++i) {
        if (servers_[i]) {
            servers_[i]->Kill();
            servers_[i].reset();
        }
    }
}

void SCTestConfig::startServer(int i) {
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
        if (servers_[i]) {
            servers_[i]->Kill();
            servers_[i].reset();
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

    auto sc = ShardCtrler::Make(peerList, i, saved_[i]);

    // Wire up raft peers
    for (int j = 0; j < n_; ++j) {
        raftPeers_[j][i]->setTarget(sc->getRaft());
    }

    {
        std::lock_guard<std::mutex> lk(mu_);
        servers_[i] = sc;

        // Update all clerk peers to target the new ShardCtrler
        for (auto& [ck, peers] : clerkPeers_) {
            if (i < static_cast<int>(peers.size())) {
                peers[i]->setTarget(sc);
            }
        }
    }
}

void SCTestConfig::shutdownServer(int i) {
    disconnect(i);

    {
        std::lock_guard<std::mutex> lk(mu_);
        if (saved_[i]) {
            saved_[i] = saved_[i]->Copy();
        }
    }

    std::shared_ptr<ShardCtrler> sc;
    {
        std::lock_guard<std::mutex> lk(mu_);
        sc = servers_[i];
        servers_[i].reset();
    }

    if (sc) {
        sc->Kill();
    }
}

void SCTestConfig::connect(int i) {
    connected_[i] = true;
    for (int j = 0; j < n_; ++j) {
        if (connected_[j]) {
            raftPeers_[i][j]->setEnabled(true);
            raftPeers_[j][i]->setEnabled(true);
        }
    }
}

void SCTestConfig::disconnect(int i) {
    connected_[i] = false;
    for (int j = 0; j < n_; ++j) {
        raftPeers_[i][j]->setEnabled(false);
        raftPeers_[j][i]->setEnabled(false);
    }
}

void SCTestConfig::connectAll() {
    for (int i = 0; i < n_; ++i) connect(i);
}

std::shared_ptr<SCClerk> SCTestConfig::makeClient(const std::vector<int>& to) {
    std::lock_guard<std::mutex> lk(mu_);

    auto peers = std::vector<std::shared_ptr<InMemSCClerkPeer>>(n_);
    for (int j = 0; j < n_; ++j) {
        peers[j] = std::make_shared<InMemSCClerkPeer>();
        if (servers_[j]) {
            peers[j]->setTarget(servers_[j]);
        }
        bool inTo = std::find(to.begin(), to.end(), j) != to.end();
        peers[j]->setEnabled(inTo);
    }

    std::vector<std::shared_ptr<SCClerkPeer>> peerVec(peers.begin(), peers.end());
    auto ck = std::make_shared<SCClerk>(peerVec);
    clerkPeers_[ck.get()] = peers;
    return ck;
}

void SCTestConfig::connectClient(SCClerk* ck, const std::vector<int>& to) {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = clerkPeers_.find(ck);
    if (it == clerkPeers_.end()) return;
    for (int j : to) {
        if (j < static_cast<int>(it->second.size())) {
            it->second[j]->setEnabled(true);
        }
    }
}

void SCTestConfig::disconnectClient(SCClerk* ck, const std::vector<int>& from) {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = clerkPeers_.find(ck);
    if (it == clerkPeers_.end()) return;
    for (int j : from) {
        if (j < static_cast<int>(it->second.size())) {
            it->second[j]->setEnabled(false);
        }
    }
}

void SCTestConfig::deleteClient(std::shared_ptr<SCClerk> ck) {
    std::lock_guard<std::mutex> lk(mu_);
    clerkPeers_.erase(ck.get());
}

std::pair<bool, int> SCTestConfig::Leader() {
    std::lock_guard<std::mutex> lk(mu_);
    for (int i = 0; i < n_; ++i) {
        if (servers_[i]) {
            auto [term, isLeader] = servers_[i]->getRaft()->GetState();
            if (isLeader) return {true, i};
        }
    }
    return {false, 0};
}

} // namespace sctest

// ═══════════════════════════════════════════════════════════════
// Tests
// ═══════════════════════════════════════════════════════════════

using namespace sctest;

TEST(ShardCtrler, TestBasic) {
    const int nservers = 3;
    SCTestConfig cfg(nservers, false, "TestBasic");

    auto ck = cfg.makeClient(cfg.all());

    cfg.begin("Test: Basic leave/join ...");

    std::vector<SCConfig> cfa(6);
    cfa[0] = ck->Query(-1);

    check(ck->Query(-1), {}, "initial");

    int gid1 = 1;
    ck->Join({{gid1, {"x", "y", "z"}}});
    check(ck->Query(-1), {gid1}, "after join gid1");
    cfa[1] = ck->Query(-1);

    int gid2 = 2;
    ck->Join({{gid2, {"a", "b", "c"}}});
    check(ck->Query(-1), {gid1, gid2}, "after join gid2");
    cfa[2] = ck->Query(-1);

    auto cfx = ck->Query(-1);
    auto sa1 = cfx.Groups.at(gid1);
    ASSERT_EQ(sa1.size(), 3u);
    ASSERT_EQ(sa1[0], "x");
    ASSERT_EQ(sa1[1], "y");
    ASSERT_EQ(sa1[2], "z");
    auto sa2 = cfx.Groups.at(gid2);
    ASSERT_EQ(sa2.size(), 3u);
    ASSERT_EQ(sa2[0], "a");
    ASSERT_EQ(sa2[1], "b");
    ASSERT_EQ(sa2[2], "c");

    ck->Leave({gid1});
    check(ck->Query(-1), {gid2}, "after leave gid1");
    cfa[4] = ck->Query(-1);

    ck->Leave({gid2});
    cfa[5] = ck->Query(-1);

    cfg.end();

    cfg.begin("Test: Historical queries ...");
    for (int s = 0; s < nservers; ++s) {
        cfg.shutdownServer(s);
        for (size_t i = 0; i < cfa.size(); ++i) {
            auto c = ck->Query(cfa[i].Num);
            check_same_config(c, cfa[i], "historical");
        }
        cfg.startServer(s);
        cfg.connectAll();
    }
    cfg.end();

    cfg.begin("Test: Move ...");
    {
        int gid3 = 503;
        ck->Join({{gid3, {"3a", "3b", "3c"}}});
        int gid4 = 504;
        ck->Join({{gid4, {"4a", "4b", "4c"}}});
        for (int i = 0; i < NShards; ++i) {
            auto cf = ck->Query(-1);
            if (i < NShards / 2) {
                ck->Move(i, gid3);
                if (cf.Shards[i] != gid3) {
                    auto cf1 = ck->Query(-1);
                    ASSERT_GT(cf1.Num, cf.Num) << "Move should increase Config.Num";
                }
            } else {
                ck->Move(i, gid4);
                if (cf.Shards[i] != gid4) {
                    auto cf1 = ck->Query(-1);
                    ASSERT_GT(cf1.Num, cf.Num) << "Move should increase Config.Num";
                }
            }
        }
        auto cf2 = ck->Query(-1);
        for (int i = 0; i < NShards; ++i) {
            if (i < NShards / 2) {
                ASSERT_EQ(cf2.Shards[i], gid3)
                    << "expected shard " << i << " on gid " << gid3;
            } else {
                ASSERT_EQ(cf2.Shards[i], gid4)
                    << "expected shard " << i << " on gid " << gid4;
            }
        }
        ck->Leave({gid3});
        ck->Leave({gid4});
    }
    cfg.end();

    cfg.begin("Test: Concurrent leave/join ...");
    {
        const int npara = 10;
        std::vector<std::shared_ptr<SCClerk>> cka(npara);
        for (int i = 0; i < npara; ++i) {
            cka[i] = cfg.makeClient(cfg.all());
        }
        std::vector<int> gids(npara);
        std::vector<std::thread> threads;
        for (int xi = 0; xi < npara; ++xi) {
            gids[xi] = xi * 10 + 100;
            threads.emplace_back([&, xi]() {
                int gid = gids[xi];
                std::string sid1 = "s" + std::to_string(gid) + "a";
                std::string sid2 = "s" + std::to_string(gid) + "b";
                cka[xi]->Join({{gid + 1000, {sid1}}});
                cka[xi]->Join({{gid, {sid2}}});
                cka[xi]->Leave({gid + 1000});
            });
        }
        for (auto& t : threads) t.join();
        check(ck->Query(-1), gids, "concurrent");
    }
    cfg.end();

    cfg.begin("Test: Minimal transfers after joins ...");
    {
        const int npara = 10;  // from concurrent test above
        auto c1 = ck->Query(-1);
        for (int i = 0; i < 5; ++i) {
            int gid = npara + 1 + i;
            ck->Join({{gid, {
                std::to_string(gid) + "a",
                std::to_string(gid) + "b",
                std::to_string(gid) + "b"
            }}});
        }
        auto c2 = ck->Query(-1);
        for (int i = 1; i <= npara; ++i) {
            for (int j = 0; j < NShards; ++j) {
                if (c2.Shards[j] == i) {
                    ASSERT_EQ(c1.Shards[j], i)
                        << "non-minimal transfer after Join()s";
                }
            }
        }
    }
    cfg.end();

    cfg.begin("Test: Minimal transfers after leaves ...");
    {
        const int npara = 10;  // from concurrent test above
        for (int i = 0; i < 5; ++i) {
            ck->Leave({npara + 1 + i});
        }
        auto c3 = ck->Query(-1);
        auto c2 = ck->Query(-1);
        // Check that shards assigned to remaining groups didn't move
        for (int i = 1; i <= npara; ++i) {
            for (int j = 0; j < NShards; ++j) {
                if (c2.Shards[j] == i) {
                    ASSERT_EQ(c3.Shards[j], i)
                        << "non-minimal transfer after Leave()s";
                }
            }
        }
    }
    cfg.end();
}

TEST(ShardCtrler, TestMulti) {
    const int nservers = 3;
    SCTestConfig cfg(nservers, false, "TestMulti");

    auto ck = cfg.makeClient(cfg.all());

    cfg.begin("Test: Multi-group join/leave ...");

    std::vector<SCConfig> cfa(6);
    cfa[0] = ck->Query(-1);

    check(ck->Query(-1), {}, "initial");

    int gid1 = 1, gid2 = 2;
    ck->Join({
        {gid1, {"x", "y", "z"}},
        {gid2, {"a", "b", "c"}}
    });
    check(ck->Query(-1), {gid1, gid2}, "after join gid1,gid2");
    cfa[1] = ck->Query(-1);

    int gid3 = 3;
    ck->Join({{gid3, {"j", "k", "l"}}});
    check(ck->Query(-1), {gid1, gid2, gid3}, "after join gid3");
    cfa[2] = ck->Query(-1);

    auto cfx = ck->Query(-1);
    auto sa1 = cfx.Groups.at(gid1);
    ASSERT_EQ(sa1.size(), 3u);
    ASSERT_EQ(sa1[0], "x");
    auto sa2 = cfx.Groups.at(gid2);
    ASSERT_EQ(sa2.size(), 3u);
    ASSERT_EQ(sa2[0], "a");
    auto sa3 = cfx.Groups.at(gid3);
    ASSERT_EQ(sa3.size(), 3u);
    ASSERT_EQ(sa3[0], "j");

    ck->Leave({gid1, gid3});
    check(ck->Query(-1), {gid2}, "after leave gid1,gid3");
    cfa[3] = ck->Query(-1);

    cfx = ck->Query(-1);
    sa2 = cfx.Groups.at(gid2);
    ASSERT_EQ(sa2.size(), 3u);
    ASSERT_EQ(sa2[0], "a");

    ck->Leave({gid2});

    cfg.end();

    cfg.begin("Test: Concurrent multi leave/join ...");
    {
        const int npara = 10;
        std::vector<std::shared_ptr<SCClerk>> cka(npara);
        for (int i = 0; i < npara; ++i) {
            cka[i] = cfg.makeClient(cfg.all());
        }
        std::vector<int> gids(npara);
        std::vector<std::thread> threads;
        for (int xi = 0; xi < npara; ++xi) {
            gids[xi] = xi + 1000;
            threads.emplace_back([&, xi]() {
                int gid = gids[xi];
                cka[xi]->Join({
                    {gid, {
                        std::to_string(gid) + "a",
                        std::to_string(gid) + "b",
                        std::to_string(gid) + "c"
                    }},
                    {gid + 1000, {std::to_string(gid + 1000) + "a"}},
                    {gid + 2000, {std::to_string(gid + 2000) + "a"}}
                });
                cka[xi]->Leave({gid + 1000, gid + 2000});
            });
        }
        for (auto& t : threads) t.join();
        check(ck->Query(-1), gids, "concurrent multi");
    }
    cfg.end();

    cfg.begin("Test: Minimal transfers after multijoins ...");
    {
        auto c1 = ck->Query(-1);
        std::map<int, std::vector<std::string>> m;
        for (int i = 0; i < 5; ++i) {
            int gid = 10 + 1 + i;  // npara=10, so gid=11..15
            m[gid] = {std::to_string(gid) + "a", std::to_string(gid) + "b"};
        }
        ck->Join(m);
        auto c2 = ck->Query(-1);
        for (int i = 1; i <= 10; ++i) {
            for (int j = 0; j < NShards; ++j) {
                if (c2.Shards[j] == i) {
                    ASSERT_EQ(c1.Shards[j], i)
                        << "non-minimal transfer after multijoins";
                }
            }
        }
    }
    cfg.end();

    cfg.begin("Test: Minimal transfers after multileaves ...");
    {
        auto c1 = ck->Query(-1);
        std::vector<int> l;
        for (int i = 0; i < 5; ++i) {
            l.push_back(11 + i);
        }
        ck->Leave(l);
        auto c2 = ck->Query(-1);
        // Check that shards assigned to remaining groups didn't move
        for (int i = 1; i <= 10; ++i) {
            for (int j = 0; j < NShards; ++j) {
                if (c2.Shards[j] == i) {
                    ASSERT_EQ(c1.Shards[j], i)
                        << "non-minimal transfer after multileaves";
                }
            }
        }
    }
    cfg.end();
}

} // namespace raft
