#ifndef ERAFT_MEMORYSTORAGE_H
#define ERAFT_MEMORYSTORAGE_H

#include "Storage.h"
#include <mutex>

namespace eraft
{

class MemoryStorage : public StorageInterface {
    
public:

    MemoryStorage();

    std::tuple<eraftpb::HardState, eraftpb::ConfState> InitialState()override;

    std::vector<eraftpb::Entry> Entries(uint64_t lo, uint64_t hi) override;

    uint64_t Term(uint64_t i) override;

    uint64_t LastIndex() override;

    uint64_t FirstIndex() override;

    eraftpb::Snapshot Snapshot() override;

    void SetHardState(eraftpb::HardState &st);

    bool ApplySnapshot(eraftpb::Snapshot &snap);

    eraftpb::Snapshot CreateSnapshot(uint64_t i, eraftpb::ConfState* cs, const char* bytes);

    bool Compact(uint64_t compactIndex);

    bool Append(std::vector<eraftpb::Entry> entries);

private:
    std::mutex mutex_;

    eraftpb::HardState hardState_;

    eraftpb::Snapshot snapShot_;

    std::vector<eraftpb::Entry> ents_;

};
    
} // namespace eraft


#endif