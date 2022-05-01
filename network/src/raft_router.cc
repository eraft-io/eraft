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
#include <network/raft_router.h>
#include <spdlog/spdlog.h>

namespace network {

Router::Router() {}

std::shared_ptr<RaftPeerState> Router::Get(uint64_t regionId) {
  if (peers_.find(regionId) != peers_.end()) {
    return peers_[regionId];
  }
  return nullptr;
}

void Router::Register(std::shared_ptr<RaftPeer> peer) {
  SPDLOG_DEBUG("register peer regionId -> " + std::to_string(peer->regionId_) +
               " peer id -> " + std::to_string(peer->PeerId()) +
               " peer addr -> " + peer->meta_->addr() + " to router");

  std::shared_ptr<RaftPeerState> peerState =
      std::make_shared<RaftPeerState>(peer);
  peers_.insert(std::pair<uint64_t, std::shared_ptr<RaftPeerState> >(
      peer->regionId_, peerState));
}

void Router::Close(uint64_t regionId) { this->peers_.erase(regionId); }

bool Router::Send(uint64_t regionId, Msg m) {
  m.regionId_ = regionId;
  SPDLOG_INFO("push raft msg to peer sender, type " + m.MsgToString());
  QueueContext::GetInstance()->get_peerSender().enqueue(m);
  return true;
}

void Router::SendStore(Msg m) {
  QueueContext::GetInstance()->get_storeSender().enqueue(m);
}

RaftStoreRouter::RaftStoreRouter(std::shared_ptr<Router> r) : router_(r) {}

RaftStoreRouter::~RaftStoreRouter() {}

bool RaftStoreRouter::Send(uint64_t regionId, Msg m) {
  router_->Send(regionId, m);
}

bool RaftStoreRouter::SendRaftMessage(const raft_messagepb::RaftMessage *msg) {
  raft_messagepb::RaftMessage *raftMsg = new raft_messagepb::RaftMessage(*msg);
  SPDLOG_DEBUG("send raft message type " +
               eraft::MsgTypeToString(raftMsg->message().msg_type()));
  Msg m = Msg(MsgType::MsgTypeRaftMessage, msg->region_id(), raftMsg);
  return router_->Send(msg->region_id(), m);
}

}  // namespace network
