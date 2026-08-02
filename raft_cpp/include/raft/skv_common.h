#pragma once
// skv_common.h — ShardKV common types, enums, serialization
// Corresponds to Go: shardkv/common.go

#include "raft/kvcommon.h"     // for OperationOp, Err
#include "raft/sc_common.h"    // for SCConfig, NShards

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace raft {

// ── Constants ────────────────────────────────────────────────
constexpr auto kSKVExecuteTimeout           = std::chrono::milliseconds(500);
constexpr auto kConfigureMonitorTimeout     = std::chrono::milliseconds(100);
constexpr auto kMigrationMonitorTimeout     = std::chrono::milliseconds(50);
constexpr auto kGCMonitorTimeout            = std::chrono::milliseconds(50);
constexpr auto kEmptyEntryDetectorTimeout   = std::chrono::milliseconds(200);

// ── SKVErr ───────────────────────────────────────────────────
enum SKVErr : uint8_t {
    SKV_OK             = 0,
    SKV_ErrNoKey       = 1,
    SKV_ErrWrongGroup  = 2,
    SKV_ErrWrongLeader = 3,
    SKV_ErrOutDated    = 4,
    SKV_ErrTimeout     = 5,
    SKV_ErrNotReady    = 6
};

inline const char* skvErrToString(SKVErr e) {
    switch (e) {
        case SKV_OK:             return "OK";
        case SKV_ErrNoKey:       return "ErrNoKey";
        case SKV_ErrWrongGroup:  return "ErrWrongGroup";
        case SKV_ErrWrongLeader: return "ErrWrongLeader";
        case SKV_ErrOutDated:    return "ErrOutDated";
        case SKV_ErrTimeout:     return "ErrTimeout";
        case SKV_ErrNotReady:    return "ErrNotReady";
    }
    return "Unknown";
}

// ── ShardStatus ──────────────────────────────────────────────
enum ShardStatus : uint8_t {
    Serving   = 0,
    Pulling   = 1,
    BePulling = 2,
    GCing     = 3
};

inline const char* shardStatusToString(ShardStatus s) {
    switch (s) {
        case Serving:   return "Serving";
        case Pulling:   return "Pulling";
        case BePulling: return "BePulling";
        case GCing:     return "GCing";
    }
    return "Unknown";
}

// ── CommandType ──────────────────────────────────────────────
enum SKVCommandType : uint8_t {
    SKV_Operation     = 0,
    SKV_Configuration = 1,
    SKV_InsertShards  = 2,
    SKV_DeleteShards  = 3,
    SKV_EmptyEntry    = 4
};

inline const char* skvCmdTypeToString(SKVCommandType t) {
    switch (t) {
        case SKV_Operation:     return "Operation";
        case SKV_Configuration: return "Configuration";
        case SKV_InsertShards:  return "InsertShards";
        case SKV_DeleteShards:  return "DeleteShards";
        case SKV_EmptyEntry:    return "EmptyEntry";
    }
    return "Unknown";
}

// ── SKVCommandRequest ────────────────────────────────────────
struct SKVCommandRequest {
    std::string key;
    std::string value;
    OperationOp op       = OpGet;
    int64_t     clientId  = 0;
    int64_t     commandId = 0;

    std::string to_string() const {
        std::ostringstream oss;
        oss << "{Key:" << key << ",Value:" << value
            << ",Op:" << opToString(op)
            << ",ClientId:" << clientId
            << ",CommandId:" << commandId << "}";
        return oss.str();
    }
};

// ── SKVCommandResponse ───────────────────────────────────────
struct SKVCommandResponse {
    SKVErr      err   = SKV_OK;
    std::string value;

    std::string to_string() const {
        std::ostringstream oss;
        oss << "{Err:" << skvErrToString(err) << ",Value:" << value << "}";
        return oss.str();
    }
};

// ── SKVOperationContext (duplicate detection) ────────────────
struct SKVOperationContext {
    int64_t           maxAppliedCommandId = 0;
    SKVCommandResponse lastResponse;
};

// ── ShardOperationRequest ────────────────────────────────────
struct ShardOperationRequest {
    int              configNum = 0;
    std::vector<int> shardIDs;

    std::string to_string() const {
        std::ostringstream oss;
        oss << "{ConfigNum:" << configNum << ",ShardIDs:[";
        for (size_t i = 0; i < shardIDs.size(); ++i) {
            if (i) oss << ",";
            oss << shardIDs[i];
        }
        oss << "]}";
        return oss.str();
    }
};

// ── ShardOperationResponse ───────────────────────────────────
struct ShardOperationResponse {
    SKVErr  err       = SKV_OK;
    int     configNum = 0;
    // shardID -> (key -> value)
    std::map<int, std::map<std::string, std::string>> shards;
    // clientID -> operation context
    std::map<int64_t, SKVOperationContext>             lastOperations;

    std::string to_string() const {
        std::ostringstream oss;
        oss << "{Err:" << skvErrToString(err)
            << ",ConfigNum:" << configNum
            << ",Shards:" << shards.size()
            << ",LastOps:" << lastOperations.size() << "}";
        return oss.str();
    }
};

// ── SKVCommand (stored in Raft log) ──────────────────────────
struct SKVCommand {
    SKVCommandType type = SKV_EmptyEntry;
    std::vector<uint8_t> data;  // serialized payload

    std::string to_string() const {
        std::ostringstream oss;
        oss << "{Type:" << skvCmdTypeToString(type)
            << ",DataLen:" << data.size() << "}";
        return oss.str();
    }
};

// ── key2shard ────────────────────────────────────────────────
inline int key2shard(const std::string& key) {
    int shard = 0;
    if (!key.empty()) {
        shard = static_cast<unsigned char>(key[0]);
    }
    return shard % NShards;
}

// ═══════════════════════════════════════════════════════════════
// Serialization — skvser namespace
// ═══════════════════════════════════════════════════════════════
namespace skvser {

// Reuse scser primitives
using scser::writeInt32;
using scser::writeInt64;
using scser::writeUint8;
using scser::writeString;
using scser::readInt32;
using scser::readInt64;
using scser::readUint8;
using scser::readString;
using scser::writeGroups;
using scser::readGroups;
using scser::writeIntVec;
using scser::readIntVec;
using scser::serializeConfig;
using scser::deserializeConfig;

// ── SKVCommandRequest serialization ──────────────────────────
inline std::vector<uint8_t> serializeRequest(const SKVCommandRequest& req) {
    std::vector<uint8_t> buf;
    writeString(buf, req.key);
    writeString(buf, req.value);
    writeUint8(buf, static_cast<uint8_t>(req.op));
    writeInt64(buf, req.clientId);
    writeInt64(buf, req.commandId);
    return buf;
}

inline SKVCommandRequest deserializeRequest(const uint8_t*& p) {
    SKVCommandRequest req;
    req.key       = readString(p);
    req.value     = readString(p);
    req.op        = static_cast<OperationOp>(readUint8(p));
    req.clientId  = readInt64(p);
    req.commandId = readInt64(p);
    return req;
}

// ── SCConfig serialization (wrap for embedding) ──────────────
inline std::vector<uint8_t> serializeConfigBuf(const SCConfig& c) {
    std::vector<uint8_t> buf;
    serializeConfig(buf, c);
    return buf;
}

// ── ShardOperationRequest serialization ──────────────────────
inline std::vector<uint8_t> serializeShardOpReq(const ShardOperationRequest& req) {
    std::vector<uint8_t> buf;
    writeInt32(buf, req.configNum);
    writeIntVec(buf, req.shardIDs);
    return buf;
}

inline ShardOperationRequest deserializeShardOpReq(const uint8_t*& p) {
    ShardOperationRequest req;
    req.configNum = readInt32(p);
    req.shardIDs  = readIntVec(p);
    return req;
}

// ── ShardOperationResponse serialization ─────────────────────
inline void writeShardMap(std::vector<uint8_t>& buf,
                          const std::map<int, std::map<std::string, std::string>>& shards) {
    writeInt32(buf, static_cast<int32_t>(shards.size()));
    for (auto& [sid, kv] : shards) {
        writeInt32(buf, sid);
        writeInt32(buf, static_cast<int32_t>(kv.size()));
        for (auto& [k, v] : kv) {
            writeString(buf, k);
            writeString(buf, v);
        }
    }
}

inline std::map<int, std::map<std::string, std::string>>
readShardMap(const uint8_t*& p) {
    std::map<int, std::map<std::string, std::string>> shards;
    int32_t ns = readInt32(p);
    for (int32_t i = 0; i < ns; ++i) {
        int sid = readInt32(p);
        int32_t nk = readInt32(p);
        std::map<std::string, std::string> kv;
        for (int32_t j = 0; j < nk; ++j) {
            std::string k = readString(p);
            std::string v = readString(p);
            kv[k] = v;
        }
        shards[sid] = std::move(kv);
    }
    return shards;
}

inline void writeLastOps(std::vector<uint8_t>& buf,
                         const std::map<int64_t, SKVOperationContext>& ops) {
    writeInt32(buf, static_cast<int32_t>(ops.size()));
    for (auto& [cid, ctx] : ops) {
        writeInt64(buf, cid);
        writeInt64(buf, ctx.maxAppliedCommandId);
        writeUint8(buf, static_cast<uint8_t>(ctx.lastResponse.err));
        writeString(buf, ctx.lastResponse.value);
    }
}

inline std::map<int64_t, SKVOperationContext>
readLastOps(const uint8_t*& p) {
    std::map<int64_t, SKVOperationContext> ops;
    int32_t n = readInt32(p);
    for (int32_t i = 0; i < n; ++i) {
        int64_t cid = readInt64(p);
        SKVOperationContext ctx;
        ctx.maxAppliedCommandId = readInt64(p);
        ctx.lastResponse.err    = static_cast<SKVErr>(readUint8(p));
        ctx.lastResponse.value  = readString(p);
        ops[cid] = ctx;
    }
    return ops;
}

inline std::vector<uint8_t> serializeShardOpResp(const ShardOperationResponse& resp) {
    std::vector<uint8_t> buf;
    writeUint8(buf, static_cast<uint8_t>(resp.err));
    writeInt32(buf, resp.configNum);
    writeShardMap(buf, resp.shards);
    writeLastOps(buf, resp.lastOperations);
    return buf;
}

inline ShardOperationResponse deserializeShardOpResp(const uint8_t*& p) {
    ShardOperationResponse resp;
    resp.err            = static_cast<SKVErr>(readUint8(p));
    resp.configNum      = readInt32(p);
    resp.shards         = readShardMap(p);
    resp.lastOperations = readLastOps(p);
    return resp;
}

// ── SKVCommand serialization ─────────────────────────────────
inline std::vector<uint8_t> serializeCommand(const SKVCommand& cmd) {
    std::vector<uint8_t> buf;
    writeUint8(buf, static_cast<uint8_t>(cmd.type));
    writeInt32(buf, static_cast<int32_t>(cmd.data.size()));
    buf.insert(buf.end(), cmd.data.begin(), cmd.data.end());
    return buf;
}

inline SKVCommand deserializeCommand(const uint8_t* data, size_t /*size*/) {
    const uint8_t* p = data;
    SKVCommand cmd;
    cmd.type = static_cast<SKVCommandType>(readUint8(p));
    int32_t len = readInt32(p);
    cmd.data.assign(p, p + len);
    return cmd;
}

inline SKVCommand deserializeCommand(const std::vector<uint8_t>& data) {
    return deserializeCommand(data.data(), data.size());
}

// ── Helper: build SKVCommand from payloads ───────────────────
inline SKVCommand makeOperationCmd(const SKVCommandRequest& req) {
    SKVCommand cmd;
    cmd.type = SKV_Operation;
    cmd.data = serializeRequest(req);
    return cmd;
}

inline SKVCommand makeConfigurationCmd(const SCConfig& config) {
    SKVCommand cmd;
    cmd.type = SKV_Configuration;
    cmd.data = serializeConfigBuf(config);
    return cmd;
}

inline SKVCommand makeInsertShardsCmd(const ShardOperationResponse& resp) {
    SKVCommand cmd;
    cmd.type = SKV_InsertShards;
    cmd.data = serializeShardOpResp(resp);
    return cmd;
}

inline SKVCommand makeDeleteShardsCmd(const ShardOperationRequest& req) {
    SKVCommand cmd;
    cmd.type = SKV_DeleteShards;
    cmd.data = serializeShardOpReq(req);
    return cmd;
}

inline SKVCommand makeEmptyEntryCmd() {
    SKVCommand cmd;
    cmd.type = SKV_EmptyEntry;
    return cmd;
}

// ── Snapshot format ──────────────────────────────────────────
// shard data: for each of NShards shards:
//   int32 kv_count, for each: string key, string value
// shard status: NShards uint8
// lastOperations: same as skvser writeLastOps
// currentConfig: serialized config
// lastConfig: serialized config

inline std::vector<uint8_t> serializeSnapshot(
    const std::array<std::map<std::string, std::string>, NShards>& shards,
    const std::array<ShardStatus, NShards>& shardStatus,
    const std::map<int64_t, SKVOperationContext>& lastOperations,
    const SCConfig& currentConfig,
    const SCConfig& lastConfig)
{
    std::vector<uint8_t> buf;
    // Shard data
    for (int i = 0; i < NShards; ++i) {
        writeInt32(buf, static_cast<int32_t>(shards[i].size()));
        for (auto& [k, v] : shards[i]) {
            writeString(buf, k);
            writeString(buf, v);
        }
    }
    // Shard status
    for (int i = 0; i < NShards; ++i) {
        writeUint8(buf, static_cast<uint8_t>(shardStatus[i]));
    }
    // Last operations
    writeLastOps(buf, lastOperations);
    // Configs
    serializeConfig(buf, currentConfig);
    serializeConfig(buf, lastConfig);
    return buf;
}

inline void deserializeSnapshot(
    const uint8_t* data, size_t /*size*/,
    std::array<std::map<std::string, std::string>, NShards>& shards,
    std::array<ShardStatus, NShards>& shardStatus,
    std::map<int64_t, SKVOperationContext>& lastOperations,
    SCConfig& currentConfig,
    SCConfig& lastConfig)
{
    const uint8_t* p = data;
    // Shard data
    for (int i = 0; i < NShards; ++i) {
        shards[i].clear();
        int32_t nk = readInt32(p);
        for (int32_t j = 0; j < nk; ++j) {
            std::string k = readString(p);
            std::string v = readString(p);
            shards[i][k] = v;
        }
    }
    // Shard status
    for (int i = 0; i < NShards; ++i) {
        shardStatus[i] = static_cast<ShardStatus>(readUint8(p));
    }
    // Last operations
    lastOperations = readLastOps(p);
    // Configs
    currentConfig = deserializeConfig(p);
    lastConfig    = deserializeConfig(p);
}

} // namespace skvser
} // namespace raft
