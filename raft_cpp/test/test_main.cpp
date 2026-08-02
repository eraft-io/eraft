// test_main.cpp — Raft tests ported from Go test_test.go
// Uses Google Test framework.

#include <gtest/gtest.h>

#include <chrono>
#include <cstring>
#include <random>
#include <thread>
#include <vector>

#include "raft/config.h"
#include "raft/types.h"

using namespace raft;
using namespace std::chrono_literals;

// ── Helpers ──────────────────────────────────────────────────

static const auto kRaftElectionTimeout = 1000ms;

static std::mt19937& testRng() {
    static thread_local std::mt19937 rng(std::random_device{}());
    return rng;
}

static int randInt(int hi) {
    std::uniform_int_distribution<int> dist(0, hi - 1);
    return dist(testRng());
}

static std::vector<uint8_t> intCmd(int v) {
    std::vector<uint8_t> b(sizeof(v));
    std::memcpy(b.data(), &v, sizeof(v));
    return b;
}

static int cmdInt(const std::vector<uint8_t>& b) {
    if (b.size() < sizeof(int)) return 0;
    int v;
    std::memcpy(&v, b.data(), sizeof(v));
    return v;
}

// ── 2A: Election Tests ───────────────────────────────────────

TEST(Raft, InitialElection2A) {
    int servers = 3;
    Config cfg(servers, false, false);
    cfg.begin("Test (2A): initial election");

    cfg.checkOneLeader();
    std::this_thread::sleep_for(50ms);

    int term1 = cfg.checkTerms();
    ASSERT_GE(term1, 1);

    std::this_thread::sleep_for(2 * kRaftElectionTimeout);
    int term2 = cfg.checkTerms();
    EXPECT_EQ(term1, term2);

    cfg.checkOneLeader();
    cfg.end();
}

TEST(Raft, ReElection2A) {
    int servers = 3;
    Config cfg(servers, false, false);
    cfg.begin("Test (2A): election after network failure");

    int leader1 = cfg.checkOneLeader();
    cfg.disconnect(leader1);
    cfg.checkOneLeader();

    cfg.connect(leader1);
    int leader2 = cfg.checkOneLeader();

    cfg.disconnect(leader2);
    cfg.disconnect((leader2 + 1) % servers);
    std::this_thread::sleep_for(2 * kRaftElectionTimeout);
    cfg.checkNoLeader();

    cfg.connect((leader2 + 1) % servers);
    cfg.checkOneLeader();

    cfg.connect(leader2);
    cfg.checkOneLeader();

    cfg.end();
}

TEST(Raft, ManyElections2A) {
    int servers = 7;
    Config cfg(servers, false, false);
    cfg.begin("Test (2A): multiple elections");

    cfg.checkOneLeader();

    for (int ii = 0; ii < 10; ++ii) {
        int i1 = randInt(servers);
        int i2 = randInt(servers);
        int i3 = randInt(servers);
        cfg.disconnect(i1);
        cfg.disconnect(i2);
        cfg.disconnect(i3);
        cfg.checkOneLeader();
        cfg.connect(i1);
        cfg.connect(i2);
        cfg.connect(i3);
    }

    cfg.checkOneLeader();
    cfg.end();
}

// ── 2B: Agreement Tests ─────────────────────────────────────

TEST(Raft, BasicAgree2B) {
    int servers = 3;
    Config cfg(servers, false, false);
    cfg.begin("Test (2B): basic agreement");

    for (int index = 1; index <= 3; ++index) {
        auto [nd, _] = cfg.nCommitted(index);
        EXPECT_EQ(nd, 0) << "some have committed before Start()";

        int xindex = cfg.one(intCmd(index * 100), servers, false);
        EXPECT_EQ(xindex, index);
    }

    cfg.end();
}

TEST(Raft, FailAgree2B) {
    int servers = 3;
    Config cfg(servers, false, false);
    cfg.begin("Test (2B): agreement despite follower disconnection");

    cfg.one(intCmd(101), servers, false);

    int leader = cfg.checkOneLeader();
    cfg.disconnect((leader + 1) % servers);

    cfg.one(intCmd(102), servers - 1, false);
    cfg.one(intCmd(103), servers - 1, false);
    cfg.one(intCmd(104), servers - 1, false);
    cfg.one(intCmd(105), servers - 1, false);

    cfg.connect((leader + 1) % servers);

    cfg.one(intCmd(106), servers, true);
    std::this_thread::sleep_for(kRaftElectionTimeout);
    cfg.one(intCmd(107), servers, true);

    cfg.end();
}

TEST(Raft, FailNoAgree2B) {
    int servers = 5;
    Config cfg(servers, false, false);
    cfg.begin("Test (2B): no agreement if too many followers disconnect");

    cfg.one(intCmd(10), servers, false);

    int leader = cfg.checkOneLeader();
    cfg.disconnect((leader + 1) % servers);
    cfg.disconnect((leader + 2) % servers);
    cfg.disconnect((leader + 3) % servers);

    auto result = cfg.rafts(leader)->Start(intCmd(20));
    ASSERT_TRUE(result.isLeader);
    EXPECT_EQ(result.index, 2);

    std::this_thread::sleep_for(2 * kRaftElectionTimeout);

    auto [n, _] = cfg.nCommitted(result.index);
    EXPECT_EQ(n, 0) << "committed but no majority";

    // Repair
    cfg.connect((leader + 1) % servers);
    cfg.connect((leader + 2) % servers);
    cfg.connect((leader + 3) % servers);

    int leader2 = cfg.checkOneLeader();
    auto result2 = cfg.rafts(leader2)->Start(intCmd(30));
    ASSERT_TRUE(result2.isLeader);
    EXPECT_GE(result2.index, 2);
    EXPECT_LE(result2.index, 3);
    cfg.one(intCmd(1000), servers, true);
    cfg.end();
}

TEST(Raft, Rejoin2B) {
    int servers = 3;
    Config cfg(servers, false, false);
    cfg.begin("Test (2B): rejoin of partitioned leader");

    cfg.one(intCmd(101), servers, true);

    int leader1 = cfg.checkOneLeader();
    cfg.disconnect(leader1);

    cfg.rafts(leader1)->Start(intCmd(102));
    cfg.rafts(leader1)->Start(intCmd(103));
    cfg.rafts(leader1)->Start(intCmd(104));

    cfg.one(intCmd(103), 2, true);

    int leader2 = cfg.checkOneLeader();
    cfg.disconnect(leader2);

    cfg.connect(leader1);
    cfg.one(intCmd(104), 2, true);

    cfg.connect(leader2);
    cfg.one(intCmd(105), servers, true);

    cfg.end();
}

TEST(Raft, Backup2B) {
    int servers = 5;
    Config cfg(servers, false, false);
    cfg.begin("Test (2B): leader backs up quickly over incorrect follower logs");

    cfg.one(intCmd(randInt(100000)), servers, true);

    int leader1 = cfg.checkOneLeader();
    cfg.disconnect((leader1 + 2) % servers);
    cfg.disconnect((leader1 + 3) % servers);
    cfg.disconnect((leader1 + 4) % servers);

    for (int i = 0; i < 50; ++i) {
        cfg.rafts(leader1)->Start(intCmd(randInt(100000)));
    }
    std::this_thread::sleep_for(kRaftElectionTimeout / 2);

    cfg.disconnect(leader1);
    cfg.disconnect((leader1 + 1) % servers);

    cfg.connect((leader1 + 2) % servers);
    cfg.connect((leader1 + 3) % servers);
    cfg.connect((leader1 + 4) % servers);

    for (int i = 0; i < 50; ++i) {
        cfg.one(intCmd(randInt(100000)), 3, true);
    }

    int leader2 = cfg.checkOneLeader();
    int other = (leader1 + 2) % servers;
    if (leader2 == other) other = (leader2 + 1) % servers;
    cfg.disconnect(other);

    for (int i = 0; i < 50; ++i) {
        cfg.rafts(leader2)->Start(intCmd(randInt(100000)));
    }
    std::this_thread::sleep_for(kRaftElectionTimeout / 2);

    for (int i = 0; i < servers; ++i) cfg.disconnect(i);
    cfg.connect(leader1);
    cfg.connect((leader1 + 1) % servers);
    cfg.connect(other);

    for (int i = 0; i < 50; ++i) {
        cfg.one(intCmd(randInt(100000)), 3, true);
    }

    for (int i = 0; i < servers; ++i) cfg.connect(i);
    cfg.one(intCmd(randInt(100000)), servers, true);

    cfg.end();
}

// ── 2C: Persistence Tests ────────────────────────────────────

TEST(Raft, Persist12C) {
    int servers = 3;
    Config cfg(servers, false, false);
    cfg.begin("Test (2C): basic persistence");

    cfg.one(intCmd(11), servers, true);

    for (int i = 0; i < servers; ++i) cfg.start1(i);
    for (int i = 0; i < servers; ++i) {
        cfg.disconnect(i);
        cfg.connect(i);
    }

    cfg.one(intCmd(12), servers, true);

    int leader1 = cfg.checkOneLeader();
    cfg.disconnect(leader1);
    cfg.start1(leader1);
    cfg.connect(leader1);

    cfg.one(intCmd(13), servers, true);

    int leader2 = cfg.checkOneLeader();
    cfg.disconnect(leader2);
    cfg.one(intCmd(14), servers - 1, true);
    cfg.start1(leader2);
    cfg.connect(leader2);

    cfg.wait(4, servers, -1);

    int i3 = (cfg.checkOneLeader() + 1) % servers;
    cfg.disconnect(i3);
    cfg.one(intCmd(15), servers - 1, true);
    cfg.start1(i3);
    cfg.connect(i3);

    cfg.one(intCmd(16), servers, true);
    cfg.end();
}

TEST(Raft, Figure82C) {
    int servers = 5;
    Config cfg(servers, false, false);
    cfg.begin("Test (2C): Figure 8");

    cfg.one(intCmd(randInt(100000)), 1, true);

    int nup = servers;
    for (int iter = 0; iter < 1000; ++iter) {
        int leader = -1;
        for (int i = 0; i < servers; ++i) {
            if (cfg.rafts(i)) {
                auto result = cfg.rafts(i)->Start(intCmd(randInt(100000)));
                if (result.isLeader) leader = i;
            }
        }

        if (randInt(1000) < 100) {
            int ms = randInt(500);
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        } else {
            int ms = randInt(13);
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        }

        if (leader != -1) {
            cfg.crash1(leader);
            --nup;
        }

        if (nup < 3) {
            int s = randInt(servers);
            if (!cfg.rafts(s)) {
                cfg.start1(s);
                cfg.connect(s);
                ++nup;
            }
        }
    }

    for (int i = 0; i < servers; ++i) {
        if (!cfg.rafts(i)) {
            cfg.start1(i);
            cfg.connect(i);
        }
    }

    cfg.one(intCmd(randInt(100000)), servers, true);
    cfg.end();
}

TEST(Raft, UnreliableAgree2C) {
    int servers = 5;
    Config cfg(servers, true, false);
    cfg.begin("Test (2C): unreliable agreement");

    for (int iter = 1; iter < 50; ++iter) {
        std::vector<std::thread> threads;
        for (int j = 0; j < 4; ++j) {
            threads.emplace_back([&cfg, iter, j] {
                cfg.one(intCmd(100 * iter + j), 1, true);
            });
        }
        cfg.one(intCmd(iter), 1, true);
        for (auto& t : threads) t.join();
    }

    cfg.setUnreliable(false);
    cfg.one(intCmd(100), servers, true);
    cfg.end();
}

// ── 2D: Snapshot Tests ───────────────────────────────────────

static void snapcommon(Config& cfg, bool disconnect, bool reliable, bool crash) {
    int iters = 30;
    int servers = cfg.n();

    cfg.one(intCmd(randInt(100000)), servers, true);
    int leader1 = cfg.checkOneLeader();

    for (int i = 0; i < iters; ++i) {
        int victim = (leader1 + 1) % servers;
        int sender = leader1;
        if (i % 3 == 1) {
            sender = (leader1 + 1) % servers;
            victim = leader1;
        }

        if (disconnect) {
            cfg.disconnect(victim);
            cfg.one(intCmd(randInt(100000)), servers - 1, true);
        }
        if (crash) {
            cfg.crash1(victim);
            cfg.one(intCmd(randInt(100000)), servers - 1, true);
        }

        for (int j = 0; j < 11; ++j) {
            cfg.rafts(sender)->Start(intCmd(randInt(100000)));
        }
        cfg.one(intCmd(randInt(100000)), servers - 1, true);

        EXPECT_LT(cfg.LogSize(), 2000) << "Log size too large";

        if (disconnect) {
            cfg.connect(victim);
            cfg.one(intCmd(randInt(100000)), servers, true);
            leader1 = cfg.checkOneLeader();
        }
        if (crash) {
            cfg.start1(victim);
            cfg.connect(victim);
            cfg.one(intCmd(randInt(100000)), servers, true);
            leader1 = cfg.checkOneLeader();
        }
    }
}

TEST(Raft, SnapshotBasic2D) {
    Config cfg(3, false, true);
    cfg.begin("Test (2D): snapshots basic");
    snapcommon(cfg, false, true, false);
    cfg.end();
}

TEST(Raft, SnapshotInstall2D) {
    Config cfg(3, false, true);
    cfg.begin("Test (2D): install snapshots (disconnect)");
    snapcommon(cfg, true, true, false);
    cfg.end();
}

TEST(Raft, SnapshotInstallUnreliable2D) {
    Config cfg(3, true, true);
    cfg.begin("Test (2D): install snapshots (disconnect+unreliable)");
    snapcommon(cfg, true, false, false);
    cfg.end();
}

TEST(Raft, SnapshotInstallCrash2D) {
    Config cfg(3, false, true);
    cfg.begin("Test (2D): install snapshots (crash)");
    snapcommon(cfg, false, true, true);
    cfg.end();
}

TEST(Raft, SnapshotInstallUnCrash2D) {
    Config cfg(3, false, true);
    cfg.begin("Test (2D): install snapshots (unreliable+crash)");
    snapcommon(cfg, false, false, true);
    cfg.end();
}
