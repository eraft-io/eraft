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

#include <Kv/router.h>
#include <Logger/logger.h>
#include <RaftCore/util.h>

namespace kvserver {

Router::Router() {}

std::shared_ptr<PeerState_> Router::Get(uint64_t regionID) {
  if (this->peers_.find(regionID) != this->peers_.end()) {
    return this->peers_[regionID];
  }
  return nullptr;
}

void Router::Register(std::shared_ptr<Peer> peer) {
  Logger::GetInstance()->DEBUG_NEW(
      "register peer regionId -> " + std::to_string(peer->regionId_) +
          " peer id -> " + std::to_string(peer->PeerId()) + " peer addr -> " +
          peer->meta_->addr() + " to router",
      __FILE__, __LINE__, "Router::Register");

  std::shared_ptr<PeerState_> ps = std::make_shared<PeerState_>(peer);
  this->peers_[peer->regionId_] = ps;
}

void Router::Close(uint64_t regionID) {
  // this->peers_[regionID]->closed_.store(1, std::memory_order_relaxed);
  this->peers_.erase(regionID);
}

//
bool Router::Send(uint64_t regionID, Msg msg) {
  msg.regionId_ = regionID;
  // std::shared_ptr<PeerState_> ps = this->Get(regionID);
  // if(ps == nullptr) {
  //     return false; // TODO: log peer not found
  // }
  Logger::GetInstance()->DEBUG_NEW(
      "push raft msg to peer sender, type " + msg.MsgToString(), __FILE__,
      __LINE__, "Router::Register");
  QueueContext::GetInstance()->peerSender_.Push(msg);
  return true;
}

void Router::SendStore(Msg m) {
  QueueContext::GetInstance()->storeSender_.Push(m);
}

bool RaftstoreRouter::Send(uint64_t regionID, const Msg m) {
  this->router_->Send(regionID, m);
}

raft_serverpb::RaftMessage* RaftstoreRouter::raft_msg_ = nullptr;

bool RaftstoreRouter::SendRaftMessage(const raft_serverpb::RaftMessage* msg) {
  if (RaftstoreRouter::raft_msg_ != nullptr) {
    delete RaftstoreRouter::raft_msg_;
  }
  RaftstoreRouter::raft_msg_ = new raft_serverpb::RaftMessage(*msg);
  Logger::GetInstance()->DEBUG_NEW(
      "send raft message type " +
          eraft::MsgTypeToString(
              RaftstoreRouter::raft_msg_->message().msg_type()) +
          " MSG_DATA: " + RaftstoreRouter::raft_msg_->message().temp_data(),
      __FILE__, __LINE__, "RaftstoreRouter::SendRaftMessage");
  Msg m = Msg(MsgType::MsgTypeRaftMessage, msg->region_id(),
              RaftstoreRouter::raft_msg_);
  return this->router_->Send(msg->region_id(), m);
}

bool RaftstoreRouter::SendRaftCommand(const kvrpcpb::RawPutRequest* put) {
  Logger::GetInstance()->DEBUG_NEW("send raft cmd message", __FILE__, __LINE__,
                                   "RaftstoreRouter::SendRaftCommand");
  // MsgRaftCmd* cmd = new MsgRaftCmd(req, cb);
  Msg m(MsgType::MsgTypeRaftCmd, 1, const_cast<kvrpcpb::RawPutRequest*>(put));
  return this->router_->Send(1, m);
}

RaftstoreRouter::RaftstoreRouter(std::shared_ptr<Router> r) {
  this->router_ = r;
}

RaftstoreRouter::~RaftstoreRouter() {}

}  // namespace kvserver
