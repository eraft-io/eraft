// sc_statemachine.cpp — MemoryConfigStateMachine implementation
// Corresponds to Go: shardctrler/configstm.go

#include "raft/sc_statemachine.h"

namespace raft {

// ── Helper functions ─────────────────────────────────────────

std::map<int, std::vector<std::string>> DeepCopyGroups(
    const std::map<int, std::vector<std::string>>& groups)
{
    std::map<int, std::vector<std::string>> result;
    for (auto& [gid, srvs] : groups) {
        result[gid] = srvs;  // vector copy
    }
    return result;
}

std::map<int, std::vector<int>> Group2Shards(const SCConfig& config) {
    std::map<int, std::vector<int>> s2g;
    // Initialize all groups with empty shard lists
    for (auto& [gid, _] : config.Groups) {
        s2g[gid];  // create empty entry
    }
    // Map each shard to its group
    for (int shard = 0; shard < NShards; ++shard) {
        int gid = config.Shards[shard];
        s2g[gid].push_back(shard);
    }
    return s2g;
}

int GetGIDWithMinimumShards(const std::map<int, std::vector<int>>& s2g) {
    // std::map is already sorted by key, so iteration is deterministic
    int index = -1;
    int min = NShards + 1;
    for (auto& [gid, shards] : s2g) {
        if (gid != 0 && static_cast<int>(shards.size()) < min) {
            index = gid;
            min = static_cast<int>(shards.size());
        }
    }
    return index;
}

int GetGIDWithMaximumShards(const std::map<int, std::vector<int>>& s2g) {
    // Always choose gid 0 if there is any
    auto it = s2g.find(0);
    if (it != s2g.end() && !it->second.empty()) {
        return 0;
    }
    // std::map is already sorted by key
    int index = -1;
    int max = -1;
    for (auto& [gid, shards] : s2g) {
        if (static_cast<int>(shards.size()) > max) {
            index = gid;
            max = static_cast<int>(shards.size());
        }
    }
    return index;
}

// ── MemoryConfigStateMachine ─────────────────────────────────

MemoryConfigStateMachine::MemoryConfigStateMachine() {
    configs_.push_back(DefaultSCConfig());
}

SCErr MemoryConfigStateMachine::Join(
    const std::map<int, std::vector<std::string>>& groups)
{
    const auto& lastConfig = configs_.back();
    SCConfig newConfig;
    newConfig.Num    = lastConfig.Num + 1;
    newConfig.Shards = lastConfig.Shards;
    newConfig.Groups = DeepCopyGroups(lastConfig.Groups);

    // Add new groups (only if not already present)
    for (auto& [gid, srvs] : groups) {
        if (newConfig.Groups.find(gid) == newConfig.Groups.end()) {
            newConfig.Groups[gid] = srvs;
        }
    }

    // Balance shards
    auto s2g = Group2Shards(newConfig);
    for (;;) {
        int source = GetGIDWithMaximumShards(s2g);
        int target = GetGIDWithMinimumShards(s2g);
        if (source != 0 &&
            static_cast<int>(s2g[source].size()) -
                static_cast<int>(s2g[target].size()) <= 1) {
            break;
        }
        // Move one shard from source to target
        s2g[target].push_back(s2g[source][0]);
        s2g[source].erase(s2g[source].begin());
    }

    // Build new shard assignment
    std::array<int, NShards> newShards = {};
    for (auto& [gid, shards] : s2g) {
        for (int shard : shards) {
            newShards[shard] = gid;
        }
    }
    newConfig.Shards = newShards;
    configs_.push_back(newConfig);
    return SC_OK;
}

SCErr MemoryConfigStateMachine::Leave(const std::vector<int>& gids) {
    const auto& lastConfig = configs_.back();
    SCConfig newConfig;
    newConfig.Num    = lastConfig.Num + 1;
    newConfig.Shards = lastConfig.Shards;
    newConfig.Groups = DeepCopyGroups(lastConfig.Groups);

    auto s2g = Group2Shards(newConfig);
    std::vector<int> orphanShards;

    for (int gid : gids) {
        // Remove group
        newConfig.Groups.erase(gid);
        // Collect orphan shards
        auto it = s2g.find(gid);
        if (it != s2g.end()) {
            orphanShards.insert(orphanShards.end(),
                                it->second.begin(), it->second.end());
            s2g.erase(it);
        }
    }

    std::array<int, NShards> newShards = {};
    if (!newConfig.Groups.empty()) {
        // Redistribute orphan shards
        for (int shard : orphanShards) {
            int target = GetGIDWithMinimumShards(s2g);
            s2g[target].push_back(shard);
        }
        for (auto& [gid, shards] : s2g) {
            for (int shard : shards) {
                newShards[shard] = gid;
            }
        }
    }
    // If no groups left, all shards go to gid 0 (newShards already 0)

    newConfig.Shards = newShards;
    configs_.push_back(newConfig);
    return SC_OK;
}

SCErr MemoryConfigStateMachine::Move(int shard, int gid) {
    const auto& lastConfig = configs_.back();
    SCConfig newConfig;
    newConfig.Num    = lastConfig.Num + 1;
    newConfig.Shards = lastConfig.Shards;
    newConfig.Groups = DeepCopyGroups(lastConfig.Groups);

    newConfig.Shards[shard] = gid;
    configs_.push_back(newConfig);
    return SC_OK;
}

std::pair<SCConfig, SCErr> MemoryConfigStateMachine::Query(int num) {
    if (num < 0 || num >= static_cast<int>(configs_.size())) {
        return {configs_.back(), SC_OK};
    }
    return {configs_[num], SC_OK};
}

} // namespace raft
