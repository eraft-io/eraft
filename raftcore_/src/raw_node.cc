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

#include <raftcore/raw_node.h>
#include <raftcore/util.h>
#include <spdlog/spdlog.h>

namespace eraft {

RawNode::RawNode(Config& config) {
  this->raft = std::make_shared<RaftContext>(config);
  this->prevSoftSt = this->raft->SoftState();
  this->prevHardSt = this->raft->HardState();
}

void RawNode::Tick() { this->raft->Tick(); }

void RawNode::Campaign() {
  SPDLOG_INFO("rawnode start campaign");
  eraftpb::Message msg;
  msg.set_msg_type(eraftpb::MsgHup);
  this->raft->Step(msg);
}

void RawNode::Propose(std::string data) {
  eraftpb::Message msg;
  msg.set_msg_type(eraftpb::MsgPropose);
  eraftpb::Entry* ent = msg.add_entries();
  ent->set_data(data);
  ent->set_entry_type(eraftpb::EntryNormal);
  this->raft->Step(msg);
}

// ...
void RawNode::ProposeConfChange(eraftpb::ConfChange cc) {
  std::string data = cc.SerializeAsString();
  SPDLOG_INFO("ProposeConfChange: " + data);
  eraftpb::Message msg;
  msg.set_msg_type(eraftpb::MsgPropose);
  eraftpb::Entry* ent = msg.add_entries();
  ent->set_data(data);
  ent->set_entry_type(eraftpb::EntryConfChange);
  this->raft->Step(msg);
}

void RawNode::ProposeSplitRegion(metapb::Region region) {
  std::string data = region.SerializeAsString();
  SPDLOG_INFO("ProposeSplitRegion: " + data);
  eraftpb::Message msg;
  msg.set_msg_type(eraftpb::MsgPropose);
  eraftpb::Entry* ent = msg.add_entries();
  ent->set_data(data);
  ent->set_entry_type(eraftpb::EntrySplitRegion);
  this->raft->Step(msg);
}

eraftpb::ConfState RawNode::ApplyConfChange(eraftpb::ConfChange cc) {
  eraftpb::ConfState confState;
  if (cc.node_id() == NONE) {
    std::vector<uint64_t> nodes = this->raft->Nodes(this->raft);
    for (uint64_t i = 0; i < nodes.size(); i++) {
      confState.set_nodes(i, nodes[i]);
    }
  }
  switch (cc.change_type()) {
    case eraftpb::AddNode: {
      this->raft->AddNode(cc.node_id());
      break;
    }
    case eraftpb::RemoveNode: {
      this->raft->RemoveNode(cc.node_id());
      break;
    }
  }
  // std::vector<uint64_t> nodes = this->raft->Nodes(this->raft);
  // for (uint64_t i = 0; i < nodes.size(); i++) {
  //   confState.set_nodes(i, nodes[i]);
  // }
  return confState;
}

void RawNode::Step(eraftpb::Message m) { this->raft->Step(m); }

DReady RawNode::EReady() {
  std::shared_ptr<RaftContext> r = this->raft;
  DReady rd;
  rd.entries = r->raftLog_->UnstableEntries();
  rd.committedEntries = r->raftLog_->NextEnts();
  rd.messages = r->msgs_;
  // if(!r->SoftState()->Equal(this->prevSoftSt)) {
  //     this->prevSoftSt = r->SoftState();
  //     rd.softSt = r->SoftState();
  // }
  // if(!IsHardStateEqual(*r->HardState(), *this->prevHardSt)) {
  //     rd.hardSt = *r->HardState();
  // }
  this->raft->msgs_.clear();
  if (!IsEmptySnap(r->raftLog_->pendingSnapshot_)) {
    rd.snapshot = r->raftLog_->pendingSnapshot_;
    r->raftLog_->pendingSnapshot_.clear_data();
  }
  return rd;
}

bool RawNode::HasReady() {
  if (!IsEmptyHardState(*this->raft->HardState()) &&
      !IsHardStateEqual(*this->raft->HardState(), *this->prevHardSt)) {
    return true;
  }
  if (this->raft->raftLog_->UnstableEntries().size() > 0 ||
      this->raft->raftLog_->NextEnts().size() > 0 ||
      this->raft->msgs_.size() > 0) {
    return true;
  }
  if (!IsEmptySnap(this->raft->raftLog_->pendingSnapshot_)) {
    return true;
  }
  return false;
}

void RawNode::Advance(DReady rd) {
  if (!IsEmptyHardState(rd.hardSt)) {
    this->prevHardSt = std::make_shared<eraftpb::HardState>(rd.hardSt);
  }
  if (rd.entries.size() > 0) {
    this->raft->raftLog_->stabled_ = rd.entries[rd.entries.size() - 1].index();
  }
  if (rd.committedEntries.size() > 0) {
    this->raft->raftLog_->applied_ =
        rd.committedEntries[rd.committedEntries.size() - 1].index();
  }
  this->raft->raftLog_->MaybeCompact();
}

std::map<uint64_t, Progress> RawNode::GetProgress() {
  std::map<uint64_t, Progress> m;
  if (this->raft->state_ == NodeState::StateLeader) {
    for (auto p : this->raft->prs_) {
      m[p.first] = *p.second;
    }
  }
  return m;
}

void RawNode::TransferLeader(uint64_t transferee) {
  eraftpb::Message msg;
  msg.set_msg_type(eraftpb::MsgTransferLeader);
  msg.set_from(transferee);
  this->Step(msg);
}

}  // namespace eraft
