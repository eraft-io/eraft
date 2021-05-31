// Copyright 2015 The etcd Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// @file Util.h
// @author Colin
// This module some util class.
// 
// Inspired by etcd golang version.

#ifndef ERAFT_RAFTCORE_UTIL_H
#define ERAFT_RAFTCORE_UTIL_H

#include <eraftio/eraftpb.pb.h>
#include <stdint.h>
#include <random>

namespace eraft
{

// IsEmptySnap returns true if the given Snapshot is empty.
bool IsEmptySnap(eraftpb::Snapshot* sp) {
    if(sp == nullptr || sp->has_metadata()) {
        return true;
    }
    return sp->metadata().index() == 0;
}

// RandIntn return a random number between [0, n)
bool RandIntn(uint64_t n) {
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, n);
    return distribution(generator);
}

// IsEmptyHardState returns true if the given HardState is empty.
bool IsEmptyHardState(eraftpb::HardState st) {
    if((st.vote() == 0) 
         && (st.term() == 0) 
         && (st.commit() == 0)) {
        return true;
    }
    return false;
}

// IsEmptyHardState returns true if t hardstate a is equal to b.
bool IsHardStateEqual(eraftpb::HardState a, eraftpb::HardState b) {
    return (a.term() == b.term() && a.vote() == b.vote() && a.commit() == b.commit());
}

} // namespace eraft


#endif // ERAFT_RAFTCORE_UTIL_H