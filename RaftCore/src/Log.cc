// @file Log.cc
// @author Colin
// This module impl the eraft::RawNode class.
// 
// Inspired by etcd golang version.

#include <RaftCore/Log.h>
#include <RaftCore/Util.h>
#include <algorithm>

// RaftLog manage the log entries, its struct look like:
//
//  snapshot/first.....applied....committed....stabled.....last
//  --------|------------------------------------------------|
//                            log entries
//
// for simplify the RaftLog implement should manage all log entries
// that not truncated

namespace eraft
{
    // newLog returns log using the given storage. It recovers the log
    // to the state that it just commits and applies the latest snapshot.
    RaftLog::RaftLog(StorageInterface &st) {
        uint64_t lo = st.FirstIndex();
        uint64_t hi = st.LastIndex();
        // std::cout << "RaftLog::st.LastIndex() = " << st.LastIndex() << std::endl;
        std::vector<eraftpb::Entry> entries = st.Entries(lo, hi + 1);
        this->storage_ = &st;
        this->entries_ = entries;
        this->applied_ = lo - 1;
        this->stabled_ = hi;
        this->firstIndex_ = lo;
        this->commited_ = 0;
    }

    RaftLog::~RaftLog() {}

    // We need to compact the log entries in some point of time like
    // storage compact stabled log entries prevent the log entries
    // grow unlimitedly in memory
    void RaftLog::MaybeCompact() {
        uint64_t first = this->storage_->FirstIndex();
        if(first > this->firstIndex_) {
            if (this->entries_.size() > 0) {
                this->entries_.erase(this->entries_.begin(), this->entries_.begin() + this->ToSliceIndex(first));
            }
            this->firstIndex_ = first;
        }
    }

    std::vector<eraftpb::Entry> RaftLog::UnstableEntries() {
        if(this->entries_.size() > 0) {
            return std::vector<eraftpb::Entry> {this->entries_.begin() + 
            (this->stabled_ - this->firstIndex_ + 1), this->entries_.end()};
        }
        return std::vector<eraftpb::Entry>{};
    }

    std::vector<eraftpb::Entry> RaftLog::NextEnts() {
        if(this->entries_.size() > 0) {
            return std::vector<eraftpb::Entry> {this->entries_.begin() + (this->applied_ - this->firstIndex_ + 1), 
            this->entries_.begin() + this->commited_ - this->firstIndex_ + 1};
        }
        return this->entries_;
    }

    uint64_t RaftLog::ToSliceIndex(uint64_t i) {
        uint64_t idx = i - this->firstIndex_;
        if(idx < 0) {
            // TODO: log panic
            exit(-1);
        }
        return idx;
    }

    uint64_t RaftLog::ToEntryIndex(uint64_t i) {
        return i + this->firstIndex_;
    }

    uint64_t RaftLog::LastIndex() {
        uint64_t index = 0;
        if(!IsEmptySnap(pendingSnapshot_)) {
            index = pendingSnapshot_.metadata().index();
        }
        if(this->entries_.size() > 0) {
            return std::max(this->entries_[this->entries_.size() - 1].index(), index);
        }
        uint64_t i = this->storage_->LastIndex();
        return std::max(i, index);
    }

    uint64_t RaftLog::Term(uint64_t i) {
        if(this->entries_.size() > 0 && i >= this->firstIndex_) {
            // TODO: check out of range
            return this->entries_[i-this->firstIndex_].term();
        }
        uint64_t term = this->storage_->Term(i);
        if(term == 0 && !IsEmptySnap(pendingSnapshot_)) {
            if (i == pendingSnapshot_.metadata().index()) {
                term = pendingSnapshot_.metadata().index();
            }
        }
        return term;
    }

} // namespace eraft
