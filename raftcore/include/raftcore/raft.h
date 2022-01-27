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

#ifndef ERAFT_RAFTCORE_RAFT_H
#define ERAFT_RAFTCORE_RAFT_H

#include <eraftio/eraftpb.pb.h>
#include <raftcore/log.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

namespace eraft {

const static uint8_t NONE = 0;

class ESoftState {
 public:
  ESoftState(uint64_t lead_, NodeState raftState) {
    this->lead = lead_;
    this->raftState = raftState;
  }

  uint64_t lead;

  NodeState raftState;

  bool Equal(std::shared_ptr<ESoftState> b) {
    return this->lead = b->lead && this->raftState == b->raftState;
  }
};

class StateMachine {
 public:
  // Step the entrance of handle message, see `MessageType`
  // on `eraftpb.proto` for what msgs should be handled
  virtual bool Step(eraftpb::Message m) = 0;

  virtual std::vector<eraftpb::Message> ReadMessage() = 0;
};

struct Config {
  Config(uint64_t id, std::vector<uint64_t> peers, uint64_t election,
         uint64_t heartbeat, std::shared_ptr<StorageInterface> st) {
    this->id = id;
    this->peers = peers;
    this->electionTick = election;
    this->heartbeatTick = heartbeat;
    this->storage = st;
    this->applied = 0;
  }

  Config(uint64_t id, uint64_t election, uint64_t heartbeat,
         uint64_t appliedIndex, std::shared_ptr<StorageInterface> st) {
    this->id = id;
    this->electionTick = election;
    this->heartbeatTick = heartbeat;
    this->applied = appliedIndex;
    this->storage = st;
  }

  // ID is the identity of the local raft. ID cannot be 0.
  uint64_t id;

  // peers contains the IDs of all nodes (including self) in the raft cluster.
  // It should only be set when starting a new raft cluster. Restarting raft
  // from previous configuration will panic if peers is set. peer is private and
  // only used for testing right now.
  std::vector<uint64_t> peers;

  // ElectionTick is the number of Node.Tick invocations that must pass between
  // elections. That is, if a follower does not receive any message from the
  // leader of current term before ElectionTick has elapsed, it will become
  // candidate and start an election. ElectionTick must be greater than
  // HeartbeatTick. We suggest ElectionTick = 10 * HeartbeatTick to avoid
  // unnecessary leader switching.
  uint64_t electionTick;

  // HeartbeatTick is the number of Node.Tick invocations that must pass between
  // heartbeats. That is, a leader sends heartbeat messages to maintain its
  // leadership every HeartbeatTick ticks.
  uint64_t heartbeatTick;

  // Storage is the storage for raft. raft generates entries and states to be
  // stored in storage. raft reads the persisted entries and states out of
  // Storage when it needs. raft reads out the previous state and configuration
  // out of storage when restarting.
  std::shared_ptr<StorageInterface> storage;

  // Applied is the last applied index. It should only be set when restarting
  // raft. raft will not return entries to the application smaller or equal to
  // Applied. If Applied is unset when restarting, raft might return previous
  // applied entries. This is a very application dependent configuration.
  uint64_t applied;

  // validate config
  bool Validate();
};

struct Progress {
  Progress(uint64_t next, uint64_t match) {
    this->next = next;
    this->match = match;
  }

  Progress(uint64_t next) {
    this->next = next;
    this->match = 0;
  }

  Progress() {
    this->next = 0;
    this->match = 0;
  }

  uint64_t match;

  uint64_t next;
};

class RaftContext : StateMachine {
 public:
  friend class RaftLog;

  friend class RawNode;

  RaftContext(Config &c);

  // Step the entrance of handle message, see `MessageType`
  // on `eraftpb.proto` for what msgs should be handled
  bool Step(eraftpb::Message m);

  // becomeCandidate transform this peer's state to candidate
  void BecomeCandidate();

  // becomeLeader transform this peer's state to leader
  void BecomeLeader();

  // becomeFollower transform this peer's state to Follower
  void BecomeFollower(uint64_t term, uint64_t lead);

  std::vector<eraftpb::Message> ReadMessage();

  // sendAppend sends an append RPC with new entries (if any) and the
  // current commit index to the given peer. Returns true if a message was sent.
  bool SendAppend(uint64_t to);

  // tick advances the internal logical clock by a single tick.
  void Tick();

  std::map<uint64_t, std::shared_ptr<Progress> > prs_;

  uint64_t id_;

  uint64_t term_;

  uint64_t vote_;

  NodeState state_;

  std::map<uint64_t, bool> votes_;

  std::shared_ptr<RaftLog> raftLog_;

  uint64_t lead_;

 private:
  std::vector<uint64_t> Nodes(std::shared_ptr<RaftContext> raft) {
    std::vector<uint64_t> nodes;
    for (auto pr : raft->prs_) {
      nodes.push_back(pr.first);
    }
    return nodes;
  }

  void SendSnapshot(uint64_t to);

  void SendAppendResponse(uint64_t to, bool reject, uint64_t term,
                          uint64_t index);

  // sendHeartbeat sends a heartbeat RPC to the given peer.
  void SendHeartbeat(uint64_t to);

  void SendHeartbeatResponse(uint64_t to, bool reject);

  void SendRequestVote(uint64_t to, uint64_t index, uint64_t term);

  void SendRequestVoteResponse(uint64_t to, bool reject);

  void SendTimeoutNow(uint64_t to);

  void TickElection();

  void TickHeartbeat();

  void TickTransfer();

  void StepFollower(eraftpb::Message m);

  void StepCandidate(eraftpb::Message m);

  void StepLeader(eraftpb::Message m);

  bool DoElection();

  void BcastHeartbeat();

  void BcastAppend();

  bool HandleRequestVote(eraftpb::Message m);

  bool HandleRequestVoteResponse(eraftpb::Message m);

  // handleAppendEntries handle AppendEntries RPC request
  bool HandleAppendEntries(eraftpb::Message m);

  bool HandleAppendEntriesResponse(eraftpb::Message m);

  void LeaderCommit();

  // handleHeartbeat handle Heartbeat RPC request
  bool HandleHeartbeat(eraftpb::Message m);

  void AppendEntries(std::vector<std::shared_ptr<eraftpb::Entry> > entries);

  std::shared_ptr<ESoftState> SoftState();

  std::shared_ptr<eraftpb::HardState> HardState();

  // handleSnapshot handle Snapshot RPC request
  bool HandleSnapshot(eraftpb::Message m);

  bool HandleTransferLeader(eraftpb::Message m);

  // addNode add a new node to raft group
  void AddNode(uint64_t id);

  // removeNode remove a node from raft group
  void RemoveNode(uint64_t id);

  std::vector<eraftpb::Message> msgs_;

  uint64_t heartbeatTimeout_;

  uint64_t electionTimeout_;

  uint64_t randomElectionTimeout_;

  // number of ticks since it reached last heartbeatTimeout.
  // only leader keeps heartbeatElapsed.
  uint64_t heartbeatElapsed_;

  // number of ticks since it reached last electionTimeout
  uint64_t electionElapsed_;

  uint64_t transferElapsed_;

  uint64_t leadTransferee_;

  uint64_t pendingConfIndex_;
};

}  // namespace eraft

#endif  // ERAFT_RAFTCORE_RAFT_H