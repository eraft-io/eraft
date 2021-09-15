// MIT License

// Copyright (c) 2021 eraft dev group

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

#include <Kv/peer.h>
#include <Kv/peer_storage.h>
#include <Kv/utils.h>
#include <RaftCore/raft.h>
#include <RaftCore/raw_node.h>
#include <eraftio/raft_serverpb.pb.h>
#include <rocksdb/write_batch.h>
#include <spdlog/spdlog.h>

#include <memory>

namespace kvserver {

// std::map<uint64_t, std::shared_ptr<metapb::Peer> > Peer::peerCache_ = {};

// std::shared_ptr<PeerStorage> Peer::peerStorage_ = nullptr;

Peer::Peer(uint64_t storeID, std::shared_ptr<Config> cfg,
           std::shared_ptr<Engines> engines,
           std::shared_ptr<metapb::Region> region)
    : stopped_(false) {
  std::shared_ptr<metapb::Peer> meta = std::make_shared<metapb::Peer>();
  // find peer
  for (auto peer : region->peers()) {
    if (peer.store_id() == storeID) {
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
  std::shared_ptr<PeerStorage> ps =
      std::make_shared<PeerStorage>(engines, region, tag);
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

  if (region->peers().size() == 1 && region->peers(0).store_id() == storeID) {
    this->raftGroup_->Campaign();
  }
}

Peer::~Peer() {}

void Peer::InsertPeerCache(metapb::Peer* peer) {
  Peer::peerCache_.insert(
      std::pair<uint64_t, std::shared_ptr<metapb::Peer> >(peer->id(), peer));
}

void Peer::RemovePeerCache(uint64_t peerID) { Peer::peerCache_.erase(peerID); }

std::shared_ptr<metapb::Peer> Peer::GetPeerFromCache(uint64_t peerID) {
  if (Peer::peerCache_.find(peerID) != Peer::peerCache_.end()) {
    return Peer::peerCache_[peerID];
  }
  for (auto peer : peerStorage_->region_->peers()) {
    if (peer.id() == peerID) {
      return std::make_shared<metapb::Peer>(peer);
    }
  }
  return nullptr;
}

uint64_t Peer::NextProposalIndex() {
  return this->raftGroup_->raft->raftLog_->LastIndex() + 1;
}

bool Peer::MaybeDestory() {
  if (this->stopped_) {
    return false;
  }
  return true;
}

bool Peer::Destory(std::shared_ptr<Engines> engine, bool keepData) {
  std::shared_ptr<metapb::Region> region = this->Region();
  std::shared_ptr<rocksdb::WriteBatch> kvWB =
      std::make_shared<rocksdb::WriteBatch>();
  std::shared_ptr<rocksdb::WriteBatch> raftWB =
      std::make_shared<rocksdb::WriteBatch>();

  if (!this->peerStorage_->ClearMeta(kvWB, raftWB)) {
    return false;
  }

  // write region state
  Assistant::GetInstance()->WriteRegionState(
      kvWB, region, raft_serverpb::PeerState::Tombstone);

  // write state to real db
  engine->kvDB_->Write(rocksdb::WriteOptions(), &*kvWB);
  engine->raftDB_->Write(rocksdb::WriteOptions(), &*raftWB);

  if (this->peerStorage_->IsInitialized() && !keepData) {
    this->peerStorage_->ClearData();
  }

  for (auto proposal : this->proposals_) {
    // TODO: notify req region removed
  }
  this->proposals_.clear();

  return true;
}

bool Peer::IsInitialized() { this->peerStorage_->IsInitialized(); }

uint64_t Peer::storeID() { return this->meta_->store_id(); }

std::shared_ptr<metapb::Region> Peer::Region() {
  return this->peerStorage_->Region();
}

void Peer::SetRegion(std::shared_ptr<metapb::Region> region) {
  this->peerStorage_->SetRegion(region);
}

uint64_t Peer::PeerId() { return this->meta_->id(); }

uint64_t Peer::LeaderId() { return this->raftGroup_->raft->lead_; }

bool Peer::IsLeader() {
  return this->raftGroup_->raft->state_ == eraft::NodeState::StateLeader;
}

void Peer::Send(std::shared_ptr<Transport> trans,
                std::vector<eraftpb::Message> msgs) {
  for (auto msg : msgs) {
    if (!this->SendRaftMessage(msg, trans)) {
      SPDLOG_ERROR("send raft msg to trans error!");
    }
  }
}

std::vector<std::shared_ptr<metapb::Peer> > Peer::CollectPendingPeers() {
  std::vector<std::shared_ptr<metapb::Peer> > pendingPeers;
  uint64_t truncatedIdx = this->peerStorage_->TruncatedIndex();
  for (auto pr : this->raftGroup_->GetProgress()) {
    if (pr.first == this->meta_->id()) {
      continue;
    }
    if (pr.second.match < truncatedIdx) {
      std::shared_ptr<metapb::Peer> peer = this->GetPeerFromCache(pr.first);
      if (peer != nullptr) {
        pendingPeers.push_back(peer);
      }
    }
  }
  return pendingPeers;
}

void Peer::ClearPeersStartPendingTime() {}

bool Peer::AnyNewPeerCatchUp(uint64_t peerId) {}

bool Peer::MaydeCampaign(bool parentIsLeader) {
  if (this->Region()->peers().size() <= 1 || !parentIsLeader) {
    return false;
  }

  this->raftGroup_->Campaign();
  return true;
}

uint64_t Peer::Term() { return this->raftGroup_->raft->term_; }

void Peer::HeartbeatScheduler() {}

bool Peer::SendRaftMessage(eraftpb::Message msg,
                           std::shared_ptr<Transport> trans) {
  std::shared_ptr<raft_serverpb::RaftMessage> sendMsg =
      std::make_shared<raft_serverpb::RaftMessage>();
  sendMsg->set_region_id(this->regionId_);
  sendMsg->mutable_region_epoch()->set_conf_ver(
      this->Region()->region_epoch().conf_ver());
  sendMsg->mutable_region_epoch()->set_version(
      this->Region()->region_epoch().version());

  auto fromPeer = this->meta_;
  auto toPeer = this->GetPeerFromCache(msg.to());
  if (toPeer == nullptr) {
    return false;
  }
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
  sendMsg->set_raft_msg_type(raft_serverpb::RaftMsgNormal);

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
  return trans->Send(sendMsg);
}

}  // namespace kvserver
