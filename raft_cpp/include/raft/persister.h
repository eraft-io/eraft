#pragma once
// persister.h — Persistent state storage
// Corresponds to Go: persister.go

#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace raft {

class Persister {
public:
    Persister() = default;

    explicit Persister(const std::string& path)
        : path_(path) { readFromDisk(); }

    // Deep copy (snapshot the current state).
    std::shared_ptr<Persister> Copy() const {
        std::lock_guard<std::mutex> lk(mu_);
        auto np = std::make_shared<Persister>();
        np->raftstate_ = raftstate_;
        np->snapshot_  = snapshot_;
        return np;
    }

    void SaveRaftState(const std::vector<uint8_t>& state) {
        std::lock_guard<std::mutex> lk(mu_);
        raftstate_ = state;
        saveToDisk();
    }

    std::vector<uint8_t> ReadRaftState() const {
        std::lock_guard<std::mutex> lk(mu_);
        return raftstate_;
    }

    size_t RaftStateSize() const {
        std::lock_guard<std::mutex> lk(mu_);
        return raftstate_.size();
    }

    // Save both Raft state and K/V snapshot atomically.
    void SaveStateAndSnapshot(const std::vector<uint8_t>& state,
                              const std::vector<uint8_t>& snapshot) {
        std::lock_guard<std::mutex> lk(mu_);
        raftstate_ = state;
        snapshot_  = snapshot;
        saveToDisk();
    }

    std::vector<uint8_t> ReadSnapshot() const {
        std::lock_guard<std::mutex> lk(mu_);
        return snapshot_;
    }

    size_t SnapshotSize() const {
        std::lock_guard<std::mutex> lk(mu_);
        return snapshot_.size();
    }

private:
    void readFromDisk() {
        if (path_.empty()) return;
        {
            std::ifstream in(path_ + ".state", std::ios::binary);
            if (in) {
                raftstate_.assign(std::istreambuf_iterator<char>(in),
                                  std::istreambuf_iterator<char>());
            }
        }
        {
            std::ifstream in(path_ + ".snapshot", std::ios::binary);
            if (in) {
                snapshot_.assign(std::istreambuf_iterator<char>(in),
                                 std::istreambuf_iterator<char>());
            }
        }
    }

    void saveToDisk() const {
        if (path_.empty()) return;
        {
            std::ofstream out(path_ + ".state",
                              std::ios::binary | std::ios::trunc);
            if (out) {
                out.write(reinterpret_cast<const char*>(raftstate_.data()),
                          static_cast<std::streamsize>(raftstate_.size()));
            }
        }
        {
            std::ofstream out(path_ + ".snapshot",
                              std::ios::binary | std::ios::trunc);
            if (out) {
                out.write(reinterpret_cast<const char*>(snapshot_.data()),
                          static_cast<std::streamsize>(snapshot_.size()));
            }
        }
    }

    mutable std::mutex mu_;
    std::vector<uint8_t> raftstate_;
    std::vector<uint8_t> snapshot_;
    std::string path_;
};

} // namespace raft
