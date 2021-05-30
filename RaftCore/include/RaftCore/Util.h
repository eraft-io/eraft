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

bool IsEmptyHardState(eraftpb::HardState st) {
    if((st.vote() == 0) 
         && (st.term() == 0) 
         && (st.commit() == 0)) {
        return true;
    }
    return false;
}

bool IsHardStateEqual(eraftpb::HardState a, eraftpb::HardState b) {
    return (a.term() == b.term() && a.vote() == b.vote() && a.commit() == b.commit());
}

} // namespace eraft


#endif // ERAFT_RAFTCORE_UTIL_H