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

//
//
// MIT License

// Copyright (c) 2021 Colin

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ERAFT_RAFTCORE_RAWNODE_H
#define ERAFT_RAFTCORE_RAWNODE_H

#include <eraftio/eraftpb.pb.h>
#include <eraftio/metapb.pb.h>
#include <raftcore/raft.h>
#include <stdint.h>

namespace eraft {

class DReady {
 public:
  std::shared_ptr<ESoftState> softSt;

  eraftpb::HardState hardSt;

  DReady() {}

  ~DReady() {}

  // Entries specifies entries to be saved to stable storage BEFORE
  // Messages are sent.
  std::vector<eraftpb::Entry> entries;

  eraftpb::Snapshot snapshot;

  std::vector<eraftpb::Entry> committedEntries;

  std::vector<eraftpb::Message> messages;
};

class RawNode {
 public:
  std::shared_ptr<RaftContext> raft;

  RawNode(Config& config);

  // Tick advances the internal logical clock by a single tick.
  void Tick();

  // Campaign causes this RawNode to transition to candidate state.
  void Campaign();

  // Propose proposes data be appended to the raft log.
  void Propose(std::string data);

  // ProposeConfChange proposes a config change.
  void ProposeConfChange(eraftpb::ConfChange cc);

  // ApplyConfChange applies a config change to the local node.
  eraftpb::ConfState ApplyConfChange(eraftpb::ConfChange cc);

  // Step advances the state machine using the given message.
  void Step(eraftpb::Message m);

  // EReady returns the current point-in-time state of this RawNode.
  DReady EReady();

  // HasReady called when RawNode user need to check if any Ready pending.
  bool HasReady();

  // Advance notifies the RawNode that the application has applied and saved
  // progress in the last Ready results.
  void Advance(DReady rd);

  // GetProgress return the the Progress of this node and its peers, if this
  // node is leader.
  std::map<uint64_t, Progress> GetProgress();

  // TransferLeader tries to transfer leadership to the given transferee.
  void TransferLeader(uint64_t transferee);

  // ProposeSplitRegion
  void ProposeSplitRegion(metapb::Region region);

 private:
  std::shared_ptr<ESoftState> prevSoftSt;

  std::shared_ptr<eraftpb::HardState> prevHardSt;
};

}  // namespace eraft

#endif  // ERAFT_RAFTCORE_RAWNODE_H