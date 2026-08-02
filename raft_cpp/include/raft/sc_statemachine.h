#pragma once
// sc_statemachine.h — ConfigStateMachine interface + MemoryConfigStateMachine
// Corresponds to Go: shardctrler/configstm.go

#include "raft/sc_common.h"

#include <map>
#include <vector>

namespace raft {

// ── ConfigStateMachine interface ─────────────────────────────
class ConfigStateMachine {
public:
    virtual ~ConfigStateMachine() = default;
    virtual SCErr Join(const std::map<int, std::vector<std::string>>& groups) = 0;
    virtual SCErr Leave(const std::vector<int>& gids) = 0;
    virtual SCErr Move(int shard, int gid) = 0;
    virtual std::pair<SCConfig, SCErr> Query(int num) = 0;
    virtual void Close() = 0;
    virtual int64_t Size() = 0;
};

// ── Helper functions for shard balancing ─────────────────────

// Invert mapping: gid -> list of shards
std::map<int, std::vector<int>> Group2Shards(const SCConfig& config);

// Find GID with minimum shards (excluding gid 0), deterministic by sorted keys
int GetGIDWithMinimumShards(const std::map<int, std::vector<int>>& s2g);

// Find GID with maximum shards (prefer gid 0 if it has shards)
int GetGIDWithMaximumShards(const std::map<int, std::vector<int>>& s2g);

// Deep copy groups map
std::map<int, std::vector<std::string>> DeepCopyGroups(
    const std::map<int, std::vector<std::string>>& groups);

// ── MemoryConfigStateMachine ─────────────────────────────────
class MemoryConfigStateMachine : public ConfigStateMachine {
public:
    MemoryConfigStateMachine();

    SCErr Join(const std::map<int, std::vector<std::string>>& groups) override;
    SCErr Leave(const std::vector<int>& gids) override;
    SCErr Move(int shard, int gid) override;
    std::pair<SCConfig, SCErr> Query(int num) override;
    void Close() override {}
    int64_t Size() override { return static_cast<int64_t>(configs_.size()); }

protected:
    std::vector<SCConfig> configs_;
};

} // namespace raft
