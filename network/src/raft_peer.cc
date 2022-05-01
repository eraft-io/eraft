// MIT License

// Copyright (c) 2022 eraft dev group

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

#include <eraftio/raft_messagepb.pb.h>
#include <network/raft_peer.h>
#include <network/raft_server_transport.h>

namespace network {

RaftPeer::RaftPeer() {}

RaftPeer::RaftPeer(uint64_t storeId, std::shared_ptr<RaftConfig> cfg,
                   std::shared_ptr<DBEngines> dbEngines,
                   std::shared_ptr<metapb::Region> region) {
  std::shared_ptr<metapb::Peer> meta = std::make_shared<metapb::Peer>();
  // find peer
  for (auto peer : region->peers()) {
    if (peer.store_id() == storeId) {
      meta->set_id(peer.id());
      meta->set_store_id(peer.store_id());
      meta->set_addr(peer.addr());
    }
  }

  if (meta->id() == 0) {
    SPDLOG_ERROR(" meta id can not be 0!");
    exit(-1);
  }

  std::string tag = "peer-" + std::to_string(meta->id());
  std::shared_ptr<RaftPeerStorage> ps =
      std::make_shared<RaftPeerStorage>(dbEngines, region, tag);
  uint64_t appliedIndex = ps->AppliedIndex();

  eraft::Config nodeConf(meta->id(), cfg->raftElectionTimeoutTicks_,
                         cfg->raftHeartbeatTicks_, appliedIndex, ps);
  std::shared_ptr<eraft::RawNode> raftGroup =
      std::make_shared<eraft::RawNode>(nodeConf);

  this->meta_ = meta;
  this->regionId_ = region->id();
  this->raftGroup_ = raftGroup;
  this->peerStorage_ = ps;
  this->peerCache_ = {};
  this->tag_ = tag;

  SPDLOG_INFO("init peer with peer id = " + std::to_string(this->meta_->id()) +
              " store id = " + std::to_string(this->meta_->store_id()) +
              " region id " + std::to_string(this->regionId_));

  if (region->peers().size() == 1 && region->peers(0).store_id() == storeId) {
    this->raftGroup_->Campaign();
  }
}

void RaftPeer::InsertPeerCache(metapb::Peer* peer) {
  RaftPeer::peerCache_.insert(
      std::pair<uint64_t, std::shared_ptr<metapb::Peer>>(peer->id(), peer));
}

void RaftPeer::RemovePeerCache(uint64_t peerId) {
  RaftPeer::peerCache_.erase(peerId);
}

std::shared_ptr<metapb::Peer> RaftPeer::GetPeerFromCache(uint64_t peerId) {
  if (RaftPeer::peerCache_.find(peerId) != RaftPeer::peerCache_.end()) {
    return RaftPeer::peerCache_[peerId];
  }
  for (auto peer : peerStorage_->region_->peers()) {
    if (peer.id() == peerId) {
      return std::make_shared<metapb::Peer>(peer);
    }
  }
  return nullptr;
}

uint64_t RaftPeer::Destory(std::shared_ptr<DBEngines> engines, bool keepData) {}

void RaftPeer::SetRegion(std::shared_ptr<metapb::Region> region) {
  this->peerStorage_->SetRegion(region);
}

uint64_t RaftPeer::PeerId() { return this->meta_->id(); }

uint64_t RaftPeer::LeaderId() { return this->raftGroup_->raft->lead_; }

bool RaftPeer::IsLeader() {
  return this->raftGroup_->raft->state_ == eraft::NodeState::StateLeader;
}

void RaftPeer::Send(std::shared_ptr<RaftServerTransport> trans,
                    std::vector<eraftpb::Message> msgs) {
  for (auto msg : msgs) {
    SendRaftMessage(msg, trans);
  }
}

uint64_t RaftPeer::Term() { return this->raftGroup_->raft->term_; }

std::shared_ptr<eraft::RawNode> RaftPeer::GetRaftGroup() {
  return this->raftGroup_;
}

uint64_t RaftPeer::GetRegionID() { return this->regionId_; }

std::shared_ptr<metapb::Region> RaftPeer::Region() {
  return this->peerStorage_->Region();
}

void RaftPeer::SendRaftMessage(eraftpb::Message msg,
                               std::shared_ptr<RaftServerTransport> trans) {
  std::shared_ptr<raft_messagepb::RaftMessage> sendMsg =
      std::make_shared<raft_messagepb::RaftMessage>();
  sendMsg->set_region_id(this->regionId_);
  sendMsg->mutable_region_epoch()->set_conf_ver(
      this->Region()->region_epoch().conf_ver());
  sendMsg->mutable_region_epoch()->set_version(
      this->Region()->region_epoch().version());

  auto fromPeer = this->meta_;
  auto toPeer = this->GetPeerFromCache(msg.to());
  sendMsg->mutable_from_peer()->set_id(fromPeer->id());
  sendMsg->mutable_from_peer()->set_store_id(fromPeer->store_id());
  sendMsg->mutable_from_peer()->set_addr(fromPeer->addr());
  sendMsg->mutable_to_peer()->set_id(toPeer->id());
  sendMsg->mutable_to_peer()->set_store_id(toPeer->store_id());
  sendMsg->mutable_to_peer()->set_addr(toPeer->addr());
  sendMsg->mutable_message()->set_from(msg.from());
  sendMsg->mutable_message()->set_to(msg.to());
  sendMsg->mutable_message()->set_index(msg.index());
  sendMsg->mutable_message()->set_term(msg.term());
  sendMsg->mutable_message()->set_commit(msg.commit());
  sendMsg->mutable_message()->set_log_term(msg.log_term());
  sendMsg->mutable_message()->set_reject(msg.reject());
  sendMsg->mutable_message()->set_msg_type(msg.msg_type());
  sendMsg->set_raft_msg_type(raft_messagepb::RaftMessageType::RaftMsgNormal);

  SPDLOG_INFO("on peer send msg" + std::to_string(msg.from()) + " to " +
              std::to_string(msg.to()) + " index " +
              std::to_string(msg.index()) + " term " +
              std::to_string(msg.term()) + " type " +
              eraft::MsgTypeToString(msg.msg_type()));
  for (auto ent : msg.entries()) {
    eraftpb::Entry* e = sendMsg->mutable_message()->add_entries();
    e->set_entry_type(ent.entry_type());
    e->set_index(ent.index());
    e->set_term(ent.term());
    e->set_data(ent.data());
  }

  // rpc
  trans->Send(sendMsg);
}

}  // namespace network
