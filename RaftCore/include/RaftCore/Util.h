#ifndef ERAFT_RAFTCORE_UTIL_H
#define ERAFT_RAFTCORE_UTIL_H

#include <eraftio/eraftpb.pb.h>

namespace eraft
{
    
bool IsEmptySnap(eraftpb::Snapshot* sp) {
    if(sp == nullptr || sp->has_metadata()) {
        return true;
    }
    return sp->metadata().index() == 0;
}

} // namespace eraft


#endif // ERAFT_RAFTCORE_UTIL_H