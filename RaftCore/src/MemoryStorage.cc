#include <RaftCore/MemoryStorage.h>
#include <tuple>

namespace eraft
{

    MemoryStorage::MemoryStorage() {
        this->ents_.reserve(1);
    }

    std::tuple<eraftpb::HardState, eraftpb::ConfState> MemoryStorage::InitialState() {
        return std::make_tuple(this->hardState_, this->snapShot_.mutable_metadata()->conf_state());
    }

    void MemoryStorage::SetHardState(eraftpb::HardState &st) {
        // TODO:
        std::lock_guard<std::mutex> lck (mutex_);
        this->hardState_ = st;
    }

    std::vector<eraftpb::Entry> MemoryStorage::Entries(uint64_t lo, uint64_t hi) {
        // TODO:
        return this->ents_;
    }

    uint64_t MemoryStorage::Term(uint64_t i) {
        // TODO:
        return 0;
    }

    uint64_t MemoryStorage::LastIndex() {
        // TODO:
        return 0;
    }

    uint64_t MemoryStorage::FirstIndex() {
        // TODO:
        return 0;
    }

    eraftpb::Snapshot MemoryStorage::Snapshot() {
        // TODO:
        return this->snapShot_;
    }

    void MemoryStorage::ApplySnapshot(eraftpb::Snapshot &snap) {
        // TODO:
    }

    eraftpb::Snapshot MemoryStorage::CreateSnapshot(uint64_t i, eraftpb::ConfState* cs, std::vector<uint8_t>& bytes) {
        // TODO:
        return this->snapShot_;
    }

    void MemoryStorage::Compact(uint64_t compactIndex) {
        // TODO:
    }

    void Append(std::vector<eraftpb::Entry> entries) {
        // TODO:
    }

} // namespace eraft
