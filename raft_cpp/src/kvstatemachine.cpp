// kvstatemachine.cpp — RocksDBKV implementation
// Corresponds to Go: kvraft/server.go (RocksDBKV)

#include "raft/kvstatemachine.h"

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/iterator.h>
#include <rocksdb/write_batch.h>

#include <filesystem>
#include <iostream>
#include <memory>

namespace raft {

RocksDBKV::RocksDBKV(const std::string& path) : path_(path) {
    opts_ = std::make_unique<rocksdb::Options>();
    opts_->create_if_missing = true;
    opts_->write_buffer_size = 64 * 1024 * 1024; // 64MB

    rocksdb::Status s = rocksdb::DB::Open(*opts_, path, &db_);
    if (!s.ok()) {
        std::cerr << "RocksDBKV: failed to open DB at " << path
                  << ": " << s.ToString() << std::endl;
        throw std::runtime_error("RocksDBKV: failed to open DB: " + s.ToString());
    }
}

RocksDBKV::~RocksDBKV() {
    Close();
}

Err RocksDBKV::Get(const std::string& key, std::string& value) {
    if (closed_) return ErrTimeout;
    rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), key, &value);
    if (s.IsNotFound()) {
        return ErrNoKey;
    }
    if (!s.ok()) {
        return ErrTimeout;
    }
    return OK;
}

Err RocksDBKV::Put(const std::string& key, const std::string& value) {
    if (closed_) return ErrTimeout;
    rocksdb::Status s = db_->Put(rocksdb::WriteOptions(), key, value);
    if (!s.ok()) {
        return ErrTimeout;
    }
    return OK;
}

Err RocksDBKV::Append(const std::string& key, const std::string& value) {
    if (closed_) return ErrTimeout;
    std::string oldValue;
    rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), key, &oldValue);
    if (!s.ok() && !s.IsNotFound()) {
        return ErrTimeout;
    }
    std::string newValue = oldValue + value;
    s = db_->Put(rocksdb::WriteOptions(), key, newValue);
    if (!s.ok()) {
        return ErrTimeout;
    }
    return OK;
}

void RocksDBKV::Close() {
    if (!closed_ && db_) {
        db_.reset();
        closed_ = true;
    }
}

int64_t RocksDBKV::Size() {
    if (closed_) return 0;
    try {
        int64_t total = 0;
        for (auto& p : std::filesystem::recursive_directory_iterator(path_)) {
            if (p.is_regular_file()) {
                total += p.file_size();
            }
        }
        return total;
    } catch (...) {
        return 0;
    }
}

std::vector<std::pair<std::string, std::string>> RocksDBKV::DumpAll() {
    std::vector<std::pair<std::string, std::string>> result;
    if (closed_) return result;

    rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        result.emplace_back(it->key().ToString(), it->value().ToString());
    }
    delete it;
    return result;
}

void RocksDBKV::BulkPut(const std::vector<std::pair<std::string, std::string>>& pairs) {
    if (closed_) return;

    // First, delete all existing keys
    rocksdb::WriteBatch batch;
    rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        batch.Delete(it->key());
    }
    delete it;

    // Then put all new pairs
    for (auto& [k, v] : pairs) {
        batch.Put(k, v);
    }

    rocksdb::Status s = db_->Write(rocksdb::WriteOptions(), &batch);
    if (!s.ok()) {
        std::cerr << "RocksDBKV::BulkPut failed: " << s.ToString() << std::endl;
    }
}

} // namespace raft
