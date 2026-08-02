#pragma once
// sc_common.h — ShardCtrler common types, enums, serialization
// Corresponds to Go: shardctrler/common.go

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
constexpr int NShards = 10;
constexpr auto kSCExecuteTimeout = std::chrono::milliseconds(500);

// ── Config ───────────────────────────────────────────────────
struct SCConfig {
    int Num = 0;
    std::array<int, NShards> Shards = {};  // shard -> gid
    std::map<int, std::vector<std::string>> Groups;  // gid -> servers[]

    SCConfig() { Shards.fill(0); }

    std::string to_string() const {
        std::ostringstream oss;
        oss << "{Num:" << Num << ",Shards:[";
        for (int i = 0; i < NShards; ++i) {
            if (i) oss << ",";
            oss << Shards[i];
        }
        oss << "],Groups:{";
        bool first = true;
        for (auto& [gid, srvs] : Groups) {
            if (!first) oss << ",";
            oss << gid << ":[";
            for (size_t i = 0; i < srvs.size(); ++i) {
                if (i) oss << ",";
                oss << srvs[i];
            }
            oss << "]";
            first = false;
        }
        oss << "}}";
        return oss.str();
    }

    bool operator==(const SCConfig& o) const {
        return Num == o.Num && Shards == o.Shards && Groups == o.Groups;
    }
    bool operator!=(const SCConfig& o) const { return !(*this == o); }
};

inline SCConfig DefaultSCConfig() {
    SCConfig c;
    c.Num = 0;
    return c;
}

// ── OperationOp ──────────────────────────────────────────────
enum SCOperationOp : uint8_t {
    SCOpJoin  = 0,
    SCOpLeave = 1,
    SCOpMove  = 2,
    SCOpQuery = 3
};

inline const char* scOpToString(SCOperationOp op) {
    switch (op) {
        case SCOpJoin:  return "OpJoin";
        case SCOpLeave: return "OpLeave";
        case SCOpMove:  return "OpMove";
        case SCOpQuery: return "OpQuery";
    }
    return "Unknown";
}

// ── Err ──────────────────────────────────────────────────────
enum SCErr : uint8_t {
    SC_OK             = 0,
    SC_ErrWrongLeader = 1,
    SC_ErrTimeout     = 2
};

inline const char* scErrToString(SCErr e) {
    switch (e) {
        case SC_OK:             return "OK";
        case SC_ErrWrongLeader: return "ErrWrongLeader";
        case SC_ErrTimeout:     return "ErrTimeout";
    }
    return "Unknown";
}

// ── CommandRequest ───────────────────────────────────────────
struct SCCommandRequest {
    std::map<int, std::vector<std::string>> Servers;  // for Join
    std::vector<int> GIDs;                             // for Leave
    int Shard = 0;                                     // for Move
    int GID   = 0;                                     // for Move
    int Num   = 0;                                     // for Query
    SCOperationOp Op = SCOpQuery;
    int64_t ClientId  = 0;
    int64_t CommandId = 0;

    std::string to_string() const {
        std::ostringstream oss;
        oss << "{";
        switch (Op) {
            case SCOpJoin:
                oss << "Servers:{...}";
                break;
            case SCOpLeave:
                oss << "GIDs:[";
                for (size_t i = 0; i < GIDs.size(); ++i) {
                    if (i) oss << ",";
                    oss << GIDs[i];
                }
                oss << "]";
                break;
            case SCOpMove:
                oss << "Shard:" << Shard << ",GID:" << GID;
                break;
            case SCOpQuery:
                oss << "Num:" << Num;
                break;
        }
        oss << ",Op:" << scOpToString(Op)
            << ",ClientId:" << ClientId << ",CommandId:" << CommandId << "}";
        return oss.str();
    }
};

// ── CommandResponse ──────────────────────────────────────────
struct SCCommandResponse {
    SCErr    Err = SC_OK;
    SCConfig Config;

    std::string to_string() const {
        std::ostringstream oss;
        oss << "{Err:" << scErrToString(Err) << ",Config:" << Config.to_string() << "}";
        return oss.str();
    }
};

// ── Command (wrapper stored in Raft log) ─────────────────────
struct SCCommand {
    SCCommandRequest Request;
};

// ── OperationContext (for duplicate detection) ───────────────
struct SCOperationContext {
    int64_t          MaxAppliedCommandId = 0;
    SCCommandResponse LastResponse;
};

// ── Serialization helpers ────────────────────────────────────
namespace scser {

inline void writeInt32(std::vector<uint8_t>& buf, int32_t v) {
    const auto* p = reinterpret_cast<const uint8_t*>(&v);
    buf.insert(buf.end(), p, p + sizeof(v));
}

inline void writeInt64(std::vector<uint8_t>& buf, int64_t v) {
    const auto* p = reinterpret_cast<const uint8_t*>(&v);
    buf.insert(buf.end(), p, p + sizeof(v));
}

inline void writeUint8(std::vector<uint8_t>& buf, uint8_t v) {
    buf.push_back(v);
}

inline void writeString(std::vector<uint8_t>& buf, const std::string& s) {
    writeInt32(buf, static_cast<int32_t>(s.size()));
    buf.insert(buf.end(), s.begin(), s.end());
}

inline int32_t readInt32(const uint8_t*& p) {
    int32_t v;
    std::memcpy(&v, p, sizeof(v));
    p += sizeof(v);
    return v;
}

inline int64_t readInt64(const uint8_t*& p) {
    int64_t v;
    std::memcpy(&v, p, sizeof(v));
    p += sizeof(v);
    return v;
}

inline uint8_t readUint8(const uint8_t*& p) {
    uint8_t v = *p;
    p += sizeof(v);
    return v;
}

inline std::string readString(const uint8_t*& p) {
    int32_t len = readInt32(p);
    std::string s(reinterpret_cast<const char*>(p), static_cast<size_t>(len));
    p += len;
    return s;
}

// Serialize map<int, vector<string>>
inline void writeGroups(std::vector<uint8_t>& buf,
                        const std::map<int, std::vector<std::string>>& groups) {
    writeInt32(buf, static_cast<int32_t>(groups.size()));
    for (auto& [gid, srvs] : groups) {
        writeInt32(buf, gid);
        writeInt32(buf, static_cast<int32_t>(srvs.size()));
        for (auto& s : srvs) writeString(buf, s);
    }
}

inline std::map<int, std::vector<std::string>> readGroups(const uint8_t*& p) {
    std::map<int, std::vector<std::string>> groups;
    int32_t n = readInt32(p);
    for (int32_t i = 0; i < n; ++i) {
        int gid = readInt32(p);
        int32_t ns = readInt32(p);
        std::vector<std::string> srvs(ns);
        for (int32_t j = 0; j < ns; ++j) srvs[j] = readString(p);
        groups[gid] = std::move(srvs);
    }
    return groups;
}

// Serialize vector<int>
inline void writeIntVec(std::vector<uint8_t>& buf, const std::vector<int>& v) {
    writeInt32(buf, static_cast<int32_t>(v.size()));
    for (int x : v) writeInt32(buf, x);
}

inline std::vector<int> readIntVec(const uint8_t*& p) {
    int32_t n = readInt32(p);
    std::vector<int> v(n);
    for (int32_t i = 0; i < n; ++i) v[i] = readInt32(p);
    return v;
}

// Serialize SCCommand (SCCommandRequest)
inline std::vector<uint8_t> serializeCommand(const SCCommand& cmd) {
    std::vector<uint8_t> buf;
    const auto& r = cmd.Request;
    writeUint8(buf, static_cast<uint8_t>(r.Op));
    writeInt64(buf, r.ClientId);
    writeInt64(buf, r.CommandId);
    writeInt32(buf, r.Shard);
    writeInt32(buf, r.GID);
    writeInt32(buf, r.Num);
    writeGroups(buf, r.Servers);
    writeIntVec(buf, r.GIDs);
    return buf;
}

inline SCCommand deserializeCommand(const uint8_t* data, size_t /*size*/) {
    const uint8_t* p = data;
    SCCommand cmd;
    auto& r = cmd.Request;
    r.Op        = static_cast<SCOperationOp>(readUint8(p));
    r.ClientId  = readInt64(p);
    r.CommandId = readInt64(p);
    r.Shard     = readInt32(p);
    r.GID       = readInt32(p);
    r.Num       = readInt32(p);
    r.Servers   = readGroups(p);
    r.GIDs      = readIntVec(p);
    return cmd;
}

inline SCCommand deserializeCommand(const std::vector<uint8_t>& data) {
    return deserializeCommand(data.data(), data.size());
}

// Serialize Config
inline void serializeConfig(std::vector<uint8_t>& buf, const SCConfig& c) {
    writeInt32(buf, c.Num);
    for (int i = 0; i < NShards; ++i) writeInt32(buf, c.Shards[i]);
    writeGroups(buf, c.Groups);
}

inline SCConfig deserializeConfig(const uint8_t*& p) {
    SCConfig c;
    c.Num = readInt32(p);
    for (int i = 0; i < NShards; ++i) c.Shards[i] = readInt32(p);
    c.Groups = readGroups(p);
    return c;
}

// Serialize SCCommandResponse
inline std::vector<uint8_t> serializeResponse(const SCCommandResponse& resp) {
    std::vector<uint8_t> buf;
    writeUint8(buf, static_cast<uint8_t>(resp.Err));
    serializeConfig(buf, resp.Config);
    return buf;
}

inline SCCommandResponse deserializeResponse(const uint8_t*& p) {
    SCCommandResponse resp;
    resp.Err    = static_cast<SCErr>(readUint8(p));
    resp.Config = deserializeConfig(p);
    return resp;
}

// Serialize SCOperationContext
inline void serializeOpContext(std::vector<uint8_t>& buf, const SCOperationContext& ctx) {
    writeInt64(buf, ctx.MaxAppliedCommandId);
    auto rb = serializeResponse(ctx.LastResponse);
    writeInt32(buf, static_cast<int32_t>(rb.size()));
    buf.insert(buf.end(), rb.begin(), rb.end());
}

inline SCOperationContext deserializeOpContext(const uint8_t*& p) {
    SCOperationContext ctx;
    ctx.MaxAppliedCommandId = readInt64(p);
    int32_t sz = readInt32(p);
    (void)sz;
    ctx.LastResponse = deserializeResponse(p);
    return ctx;
}

// Snapshot format:
//   int32 config_count
//   for each config: serialized config
//   int32 op_count
//   for each: int64 clientId, serialized opContext
inline std::vector<uint8_t> serializeSnapshot(
    const std::vector<SCConfig>& configs,
    const std::vector<std::pair<int64_t, SCOperationContext>>& ops)
{
    std::vector<uint8_t> buf;
    writeInt32(buf, static_cast<int32_t>(configs.size()));
    for (auto& c : configs) serializeConfig(buf, c);
    writeInt32(buf, static_cast<int32_t>(ops.size()));
    for (auto& [cid, ctx] : ops) {
        writeInt64(buf, cid);
        serializeOpContext(buf, ctx);
    }
    return buf;
}

inline void deserializeSnapshot(
    const uint8_t* data, size_t /*size*/,
    std::vector<SCConfig>& configs,
    std::vector<std::pair<int64_t, SCOperationContext>>& ops)
{
    const uint8_t* p = data;
    int32_t nc = readInt32(p);
    configs.resize(nc);
    for (int32_t i = 0; i < nc; ++i) configs[i] = deserializeConfig(p);
    int32_t no = readInt32(p);
    ops.resize(no);
    for (int32_t i = 0; i < no; ++i) {
        ops[i].first = readInt64(p);
        ops[i].second = deserializeOpContext(p);
    }
}

} // namespace scser
} // namespace raft
