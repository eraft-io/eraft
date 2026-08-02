#pragma once
// kvcommon.h — KV Raft common types, enums, serialization
// Corresponds to Go: kvraft/common.go

#include <chrono>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace raft {

// ── Timeout ──────────────────────────────────────────────────
constexpr auto kExecuteTimeout = std::chrono::milliseconds(500);

// ── OperationOp ──────────────────────────────────────────────
enum OperationOp : uint8_t {
    OpPut    = 0,
    OpAppend = 1,
    OpGet    = 2
};

inline const char* opToString(OperationOp op) {
    switch (op) {
        case OpPut:    return "OpPut";
        case OpAppend: return "OpAppend";
        case OpGet:    return "OpGet";
    }
    return "Unknown";
}

// ── Err ──────────────────────────────────────────────────────
enum Err : uint8_t {
    OK             = 0,
    ErrNoKey       = 1,
    ErrWrongLeader = 2,
    ErrTimeout     = 3
};

inline const char* errToString(Err e) {
    switch (e) {
        case OK:             return "OK";
        case ErrNoKey:       return "ErrNoKey";
        case ErrWrongLeader: return "ErrWrongLeader";
        case ErrTimeout:     return "ErrTimeout";
    }
    return "Unknown";
}

// ── CommandRequest ───────────────────────────────────────────
struct CommandRequest {
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

// ── CommandResponse ──────────────────────────────────────────
struct CommandResponse {
    Err         err   = OK;
    std::string value;

    std::string to_string() const {
        std::ostringstream oss;
        oss << "{Err:" << errToString(err) << ",Value:" << value << "}";
        return oss.str();
    }
};

// ── Command (wrapper stored in Raft log) ─────────────────────
struct Command {
    CommandRequest request;
};

// ── OperationContext (for duplicate detection) ───────────────
struct OperationContext {
    int64_t          maxAppliedCommandId = 0;
    CommandResponse  lastResponse;
};

// ── Serialization helpers ────────────────────────────────────
namespace kvser {

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

// Serialize a Command (CommandRequest) to bytes
inline std::vector<uint8_t> serializeCommand(const Command& cmd) {
    std::vector<uint8_t> buf;
    writeString(buf, cmd.request.key);
    writeString(buf, cmd.request.value);
    writeUint8(buf, static_cast<uint8_t>(cmd.request.op));
    writeInt64(buf, cmd.request.clientId);
    writeInt64(buf, cmd.request.commandId);
    return buf;
}

// Deserialize bytes to Command
inline Command deserializeCommand(const uint8_t* data, size_t size) {
    const uint8_t* p = data;
    Command cmd;
    cmd.request.key       = readString(p);
    cmd.request.value     = readString(p);
    cmd.request.op        = static_cast<OperationOp>(readUint8(p));
    cmd.request.clientId  = readInt64(p);
    cmd.request.commandId = readInt64(p);
    return cmd;
}

inline Command deserializeCommand(const std::vector<uint8_t>& data) {
    return deserializeCommand(data.data(), data.size());
}

// Snapshot format:
//   int32  kv_count
//   for each: string key, string value
//   int32  ops_count
//   for each: int64 clientId, int64 maxCmdId, uint8 err, string value

inline std::vector<uint8_t> serializeSnapshot(
    const std::vector<std::pair<std::string, std::string>>& kvPairs,
    const std::vector<std::pair<int64_t, OperationContext>>& ops)
{
    std::vector<uint8_t> buf;
    writeInt32(buf, static_cast<int32_t>(kvPairs.size()));
    for (auto& [k, v] : kvPairs) {
        writeString(buf, k);
        writeString(buf, v);
    }
    writeInt32(buf, static_cast<int32_t>(ops.size()));
    for (auto& [cid, ctx] : ops) {
        writeInt64(buf, cid);
        writeInt64(buf, ctx.maxAppliedCommandId);
        writeUint8(buf, static_cast<uint8_t>(ctx.lastResponse.err));
        writeString(buf, ctx.lastResponse.value);
    }
    return buf;
}

inline void deserializeSnapshot(
    const uint8_t* data, size_t /*size*/,
    std::vector<std::pair<std::string, std::string>>& kvPairs,
    std::vector<std::pair<int64_t, OperationContext>>& ops)
{
    const uint8_t* p = data;
    int32_t kvCount = readInt32(p);
    kvPairs.resize(kvCount);
    for (int32_t i = 0; i < kvCount; ++i) {
        kvPairs[i].first  = readString(p);
        kvPairs[i].second = readString(p);
    }
    int32_t opsCount = readInt32(p);
    ops.resize(opsCount);
    for (int32_t i = 0; i < opsCount; ++i) {
        ops[i].first = readInt64(p);
        ops[i].second.maxAppliedCommandId = readInt64(p);
        ops[i].second.lastResponse.err    = static_cast<Err>(readUint8(p));
        ops[i].second.lastResponse.value  = readString(p);
    }
}

} // namespace kvser
} // namespace raft
