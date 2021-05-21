#include <RaftCore/Log.h>

namespace eraft
{

    RaftLog::RaftLog(StorageInterface &st) {
        // TODO:
    }

    void RaftLog::MaybeCompact() {
        // TODO:
    }

    std::vector<eraftpb::Entry> RaftLog::UnstableEntries() {
        //TODO:

        return this->entries_;
    }

    std::vector<eraftpb::Entry> RaftLog::NextEnts() {
        // TODO:
        return this->entries_;
    }

    uint64_t RaftLog::ToSliceIndex(uint64_t i) {
        // TODO:
        return 0;
    }

    uint64_t RaftLog::ToEntryIndex(uint64_t i) {
        return 0;
    }

    uint64_t RaftLog::LastIndex() {
        return 0;
    }

    uint64_t RaftLog::Term(uint64_t i) {
        return 0;
    }

} // namespace eraft
