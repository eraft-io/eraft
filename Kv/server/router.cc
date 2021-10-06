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

#include <Kv/router.h>
#include <RaftCore/util.h>
#include <spdlog/spdlog.h>

namespace kvserver {

Router::Router() {}

std::shared_ptr<PeerState_> Router::Get(uint64_t regionID) {
  if (this->peers_.find(regionID) != this->peers_.end()) {
    return this->peers_[regionID];
  }
  return nullptr;
}

void Router::Register(std::shared_ptr<Peer> peer) {
  SPDLOG_INFO("register peer regionId -> " + std::to_string(peer->regionId_) +
              " peer id -> " + std::to_string(peer->PeerId()) +
              " peer addr -> " + peer->meta_->addr() + " to router");

  std::shared_ptr<PeerState_> ps = std::make_shared<PeerState_>(peer);
  // this->peers_[peer->regionId_] = ps;
  this->peers_.insert(
      std::pair<uint64_t, std::shared_ptr<PeerState_> >(peer->regionId_, ps));
}

void Router::Close(uint64_t regionID) { this->peers_.erase(regionID); }

//
bool Router::Send(uint64_t regionID, Msg msg) {
  msg.regionId_ = regionID;
  SPDLOG_INFO("push raft msg to peer sender, type " + msg.MsgToString());
  QueueContext::GetInstance()->get_peerSender().Push(msg);
  return true;
}

void Router::SendStore(Msg m) {
  QueueContext::GetInstance()->get_storeSender().Push(m);
}

bool RaftstoreRouter::Send(uint64_t regionID, const Msg m) {
  this->router_->Send(regionID, m);
}

raft_serverpb::RaftMessage* RaftstoreRouter::raft_msg_ = nullptr;

bool RaftstoreRouter::SendRaftMessage(const raft_serverpb::RaftMessage* msg) {
  raft_serverpb::RaftMessage* raftmsg = new raft_serverpb::RaftMessage(*msg);
  SPDLOG_INFO("send raft message type " +
              eraft::MsgTypeToString(raftmsg->message().msg_type()));
  Msg m = Msg(MsgType::MsgTypeRaftMessage, msg->region_id(), raftmsg);
  return this->router_->Send(msg->region_id(), m);
}

bool RaftstoreRouter::SendRaftCommand(const kvrpcpb::RawPutRequest* put) {
  SPDLOG_INFO("send raft cmd message");
  Msg m(MsgType::MsgTypeRaftCmd, 1, const_cast<kvrpcpb::RawPutRequest*>(put));
  return this->router_->Send(1, m);
}

RaftstoreRouter::RaftstoreRouter(std::shared_ptr<Router> r) {
  this->router_ = r;
}

RaftstoreRouter::~RaftstoreRouter() {}

}  // namespace kvserver
