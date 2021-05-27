#ifndef ERAFT_RAFTCORE_UTIL_H
#define ERAFT_RAFTCORE_UTIL_H

#include <eraftio/eraftpb.pb.h>
#include <stdint.h>
#include <random>

namespace eraft
{
    
bool IsEmptySnap(eraftpb::Snapshot* sp) {
    if(sp == nullptr || sp->has_metadata()) {
        return true;
    }
    return sp->metadata().index() == 0;
}

bool RandIntn(uint64_t n) {
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, n);
    return distribution(generator);
}

} // namespace eraft


#endif // ERAFT_RAFTCORE_UTIL_H