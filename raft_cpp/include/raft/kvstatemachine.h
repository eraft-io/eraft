#pragma once
// kvstatemachine.h — KVStateMachine interface + RocksDBKV implementation
// Corresponds to Go: kvraft/server.go (RocksDBKV)

#include "raft/kvcommon.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

// Forward-declare RocksDB types to avoid exposing them in the header
namespace rocksdb {
class DB;
class Options;
}

namespace raft {

// ── KVStateMachine interface ─────────────────────────────────
class KVStateMachine {
public:
    virtual ~KVStateMachine() = default;
    virtual Err Get(const std::string& key, std::string& value) = 0;
    virtual Err Put(const std::string& key, const std::string& value) = 0;
    virtual Err Append(const std::string& key, const std::string& value) = 0;
    virtual void Close() = 0;
    virtual int64_t Size() = 0;

    // For snapshot: dump all key-value pairs
    virtual std::vector<std::pair<std::string, std::string>> DumpAll() = 0;
    // For snapshot restore: bulk put
    virtual void BulkPut(const std::vector<std::pair<std::string, std::string>>& pairs) = 0;
};

// ── RocksDBKV ────────────────────────────────────────────────
class RocksDBKV : public KVStateMachine {
public:
    explicit RocksDBKV(const std::string& path);
    ~RocksDBKV() override;

    Err Get(const std::string& key, std::string& value) override;
    Err Put(const std::string& key, const std::string& value) override;
    Err Append(const std::string& key, const std::string& value) override;
    void Close() override;
    int64_t Size() override;

    std::vector<std::pair<std::string, std::string>> DumpAll() override;
    void BulkPut(const std::vector<std::pair<std::string, std::string>>& pairs) override;

private:
    std::unique_ptr<rocksdb::DB> db_;
    std::unique_ptr<rocksdb::Options> opts_;
    std::string       path_;
    bool              closed_ = false;
};

} // namespace raft
