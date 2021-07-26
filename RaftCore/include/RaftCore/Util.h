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
#include <stdlib.h>
#include <random>
#include <google/protobuf/text_format.h>
#include <RaftCore/Log.h>

namespace eraft
{

static std::string BoolToString(bool b)
{
  return b ? "true" : "false";
}

static std::string MsgTypeToString(eraftpb::MessageType t) {
    switch (t)
    {
    case eraftpb::MsgHup:
        return "MsgHup";
    case eraftpb::MsgBeat:
        return "MsgBeat";
    case eraftpb::MsgPropose:
        return "MsgPropose";
    case eraftpb::MsgAppend:
        return "MsgAppend";
    case eraftpb::MsgAppendResponse:
        return "MsgAppendResponse";
    case eraftpb::MsgRequestVote:
        return "MsgRequestVote";
    case eraftpb::MsgRequestVoteResponse:
        return "MsgRequestVoteResponse";
    case eraftpb::MsgSnapshot:
        return "MsgSnapshot";
    case eraftpb::MsgHeartbeat:
        return "MsgHeartbeat";
    case eraftpb::MsgHeartbeatResponse:
        return "MsgHeartbeatResponse";
    case eraftpb::MsgTransferLeader:
        return "MsgTransferLeader";
    case eraftpb::MsgTimeoutNow:
        return "MsgTimeoutNow";
    }
}

static std::string StateToString(NodeState st) {
    switch (st)
    {
    case NodeState::StateLeader:
        {
            return "StateLeader";
        }
    case NodeState::StateFollower:
        {
            return "StateFollower";
        }
    case NodeState::StateCandidate:
        {
            return "StateCandidate";
        }
    }
}

static const uint64_t kRaftInvalidIndex = 0;

static bool IsInitialMsg(eraftpb::Message& msg)
{
    return msg.msg_type() == eraftpb::MsgRequestVote || (msg.msg_type() == eraftpb::MsgHeartbeat && msg.commit() == kRaftInvalidIndex);
}

// IsEmptySnap returns true if the given Snapshot is empty.
static bool IsEmptySnap(eraftpb::Snapshot sp) {
    if(sp.has_metadata()) {
        return true;
    }
    return sp.metadata().index() == 0;
}

// RandIntn return a random number between [0, n)
static uint64_t RandIntn(uint64_t n) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist6(0, n - 1);
    return static_cast<uint64_t>(dist6(rng));
}

// IsEmptyHardState returns true if the given HardState is empty.
static bool IsEmptyHardState(eraftpb::HardState st) {
    if((st.vote() == 0) 
         && (st.term() == 0) 
         && (st.commit() == 0)) {
        return true;
    }
    return false;
}

// IsEmptyHardState returns true if t hardstate a is equal to b.
static bool IsHardStateEqual(eraftpb::HardState a, eraftpb::HardState b) {
    return (a.term() == b.term() && a.vote() == b.vote() && a.commit() == b.commit());
}

// protobuf Message to string
static std::string MessageToString(eraftpb::Message m) {
    std::string str1;
    google::protobuf::TextFormat::PrintToString(m, &str1);
    return str1;
}

// protobuf Entry to string
static std::string EntryToString(eraftpb::Entry m) {
    std::string str1;
    google::protobuf::TextFormat::PrintToString(m, &str1);
    return str1;
}

} // namespace eraft


#endif // ERAFT_RAFTCORE_UTIL_H