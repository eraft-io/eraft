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

#include <eraftio/metapb.pb.h>
#include <eraftio/raft_messagepb.pb.h>
#include <network/raft_bootstrap.h>
#include <network/raft_encode_assistant.h>
#include <network/raft_peer.h>
#include <network/raft_peer_msg_handler.h>

namespace network {

RaftPeerMsgHandler::RaftPeerMsgHandler(std::shared_ptr<RaftPeer> peer,
                                       std::shared_ptr<GlobalContext> ctx)
    : peer_(peer), ctx_(ctx) {}

RaftPeerMsgHandler::~RaftPeerMsgHandler() {}

std::shared_ptr<std::string> RaftPeerMsgHandler::GetRequestKey(
    raft_messagepb::Request *req) {
  std::string key;
  switch (req->cmd_type()) {
    case raft_messagepb::CmdType::Get: {
      key = req->get().key();
      break;
    }
    case raft_messagepb::CmdType::Put: {
      key = req->put().key();
      break;
    }
    case raft_messagepb::CmdType::Delete: {
      key = req->delete_().key();
      break;
    }
    default:
      break;
  }
  return std::make_shared<std::string>(key);
}

std::shared_ptr<storage::WriteBatch> RaftPeerMsgHandler::ProcessRequest(
    eraftpb::Entry *entry, raft_messagepb::RaftCmdRequest *msg,
    std::shared_ptr<storage::WriteBatch> wb) {
  auto req = msg->requests(0);
  std::shared_ptr<std::string> key = GetRequestKey(&req);
  SPDLOG_INFO("process request key: " + *key);
  switch (req.cmd_type()) {
    case raft_messagepb::CmdType::Put: {
      wb->Put(RaftEncodeAssistant::GetInstance()->KeyWithCF(req.put().cf(),
                                                            req.put().key()),
              req.put().value());
      SPDLOG_INFO("process put request with cf " + req.put().cf() + " key " +
                  req.put().key() + " value " + req.put().value());
      break;
    }
    case raft_messagepb::CmdType::Delete: {
      wb->Delete(RaftEncodeAssistant::GetInstance()->KeyWithCF(
          req.delete_().cf(), req.delete_().key()));
      SPDLOG_INFO("process delete request with cf " + req.delete_().cf() +
                  " key " + req.delete_().key());
      break;
    }
    default:
      break;
  }

  return wb;
}

void RaftPeerMsgHandler::ProcessConfChange(
    eraftpb::Entry *entry, eraftpb::ConfChange *cc,
    std::shared_ptr<storage::WriteBatch> wb) {
  switch (cc->change_type()) {
    case eraftpb::ConfChangeType::AddNode: {
      SPDLOG_INFO("add node");
      metapb::Peer *peer = new metapb::Peer();
      peer->set_id(cc->node_id());
      peer->set_store_id(cc->node_id());
      peer->set_addr(cc->context());
      this->peer_->InsertPeerCache(peer);
      break;
    }
    case eraftpb::ConfChangeType::RemoveNode: {
      SPDLOG_INFO("remove node");
      this->peer_->RemovePeerCache(cc->node_id());
      break;
    }
    default:
      break;
  }
  this->peer_->GetRaftGroup()->ApplyConfChange(*cc);
}

std::shared_ptr<RaftPeer> RaftPeerMsgHandler::GetRaftPeer() {
  return this->peer_;
}

void RaftPeerMsgHandler::ProcessSplitRegion(
    eraftpb::Entry *entry, metapb::Region *newRegion,
    std::shared_ptr<storage::WriteBatch> wb) {
  uint64_t regionid = this->GetRaftPeer()->GetRegionID();
  std::unique_lock<std::mutex> mlock(this->ctx_->storeMeta_->mutex_);
  if (this->ctx_->storeMeta_->regions_.find(regionid) ==
      this->ctx_->storeMeta_->regions_.end()) {
    SPDLOG_INFO("ProcessSplitRegion() error not find");
    return;
  }
  metapb::Region *currentRegion = this->ctx_->storeMeta_->regions_[regionid];
  metapb::RegionEpoch *rep = currentRegion->mutable_region_epoch();
  rep->set_version(rep->version() + 1);
  for (auto per : currentRegion->peers()) {
    metapb::Peer *newpeer = newRegion->add_peers();
    newpeer->set_addr(per.addr());
    newpeer->set_store_id(per.store_id());
    newpeer->set_id(per.id());
  }
  newRegion->mutable_region_epoch()->set_conf_ver(1);
  newRegion->mutable_region_epoch()->set_version(1);
  newRegion->set_end_key(currentRegion->end_key());
  currentRegion->set_end_key(newRegion->start_key());
  this->ctx_->storeMeta_->regions_[newRegion->id()] = newRegion;
  mlock.unlock();
  std::string debugVal;
  google::protobuf::TextFormat::PrintToString(*currentRegion, &debugVal);
  SPDLOG_INFO("oldregion: " + debugVal);
  google::protobuf::TextFormat::PrintToString(*newRegion, &debugVal);
  SPDLOG_INFO("newregion: " + debugVal);
  metapb::Region curegion = *currentRegion;
  metapb::Region newregion = *newRegion;
  RaftEncodeAssistant::GetInstance()->WriteRegionState(
      this->ctx_->engine_->kvDB_, std::make_shared<metapb::Region>(curegion),
      raft_messagepb::PeerState::Normal);
  RaftEncodeAssistant::GetInstance()->WriteRegionState(
      this->ctx_->engine_->kvDB_, std::make_shared<metapb::Region>(newregion),
      raft_messagepb::PeerState::Normal);
  BootHelper::GetInstance()->PrepareBoostrapCluster(
      this->ctx_->engine_, std::make_shared<metapb::Region>(newregion));
  // this->peer_->sizeDiffHint_ = 0;
  // this->peer_->approximateSize_ = 0;
  std::shared_ptr<RaftPeer> newpeer = std::make_shared<RaftPeer>(
      this->ctx_->store_->id(), this->ctx_->cfg_, this->ctx_->engine_,
      std::make_shared<metapb::Region>(newregion));
  this->ctx_->router_->Register(newpeer);
  Msg m(MsgType::MsgTypeStart, newregion.id(), nullptr);
  this->ctx_->router_->Send(newregion.id(), m);
}

std::shared_ptr<storage::WriteBatch> RaftPeerMsgHandler::Process(
    eraftpb::Entry *entry, std::shared_ptr<storage::WriteBatch> wb) {
  switch (entry->entry_type()) {
    case eraftpb::EntryType::EntryConfChange: {
      SPDLOG_INFO("process conf change entry");
      eraftpb::ConfChange *cc = new eraftpb::ConfChange();
      cc->ParseFromString(entry->data());
      this->ProcessConfChange(entry, cc, wb);
      delete cc;
      break;
    }
    case eraftpb::EntryType::EntryNormal: {
      raft_messagepb::RawPutRequest *msg = new raft_messagepb::RawPutRequest();
      msg->ParseFromString(entry->data());
      SPDLOG_INFO("Process Entry DATA: cf " + msg->cf() + " key " + msg->key() +
                  " val " + msg->value());
      if (msg->cf().empty() && msg->key().empty()) {
        delete msg;
        return wb;
      }
      storage::WriteBatch kvWB;
      if (msg->type() == 1) {
        kvWB.Put(RaftEncodeAssistant::GetInstance()->KeyWithCF(msg->cf(),
                                                               msg->key()),
                 msg->value());
      } else if (msg->type() == 2) {
        kvWB.Delete(RaftEncodeAssistant::GetInstance()->KeyWithCF(msg->cf(),
                                                                  msg->key()));
      }

      auto status =
          this->peer_->peerStorage_->engines_->kvDB_->PutWriteBatch(kvWB);
      if (status != storage::EngOpStatus::OK) {
        SPDLOG_INFO("err: when process entry data() cf " + msg->cf() + " key " +
                    msg->key() + " val " + msg->value() + ")");
      }
      if (this->peer_->IsLeader()) {
        // std::mutex mapMutex;
        // {
        //   std::lock_guard<std::mutex> lg(mapMutex);
        //   Server::readyCondVars_[msg->id()]
        //       ->notify_one();  // notify client process ok
        // }
      }
      delete msg;
      break;
    }
    case eraftpb::EntryType::EntrySplitRegion: {
      metapb::Region *newregion = new metapb::Region();
      newregion->ParseFromString(entry->data());
      SPDLOG_INFO("process split region split key : " + newregion->start_key() +
                  " id: " + std::to_string(newregion->id()));
      this->ProcessSplitRegion(entry, newregion, wb);
      break;
    }
    default:
      break;
  }
  return wb;
}

void RaftPeerMsgHandler::HandleRaftReady() {
  // pop ready message, send to tran
  // ServerTransport::Send(std::shared_ptr<raft_serverpb::RaftMessage> msg)
}

void RaftPeerMsgHandler::HandleMsg(Msg m) {
  // msg type do something
  SPDLOG_INFO("handle raft msg type " + m.MsgToString());
  switch (m.type_) {
    case MsgType::MsgTypeRaftMessage: {
      if (m.data_ != nullptr) {
        try {
          auto raftMsg = static_cast<raft_messagepb::RaftMessage *>(m.data_);
          if (raftMsg == nullptr) {
            SPDLOG_ERROR("handle nil message");
            return;
          }
          switch (raftMsg->raft_msg_type()) {
            case raft_messagepb::RaftMessageType::RaftMsgNormal: {
              if (!this->OnRaftMsg(raftMsg)) {
                SPDLOG_ERROR("on handle raft msg");
              }
              break;
            }
            case raft_messagepb::RaftMessageType::RaftMsgClientCmd: {
              // std::shared_ptr<kvrpcpb::RawPutRequest> put =
              //     std::make_shared<kvrpcpb::RawPutRequest>();
              // put->set_key(raftMsg->data());
              // SPDLOG_INFO("PROPOSE NEW: " + put->key());
              this->ProposeRaftCommand(raftMsg->data());
              break;
            }
            case raft_messagepb::RaftMessageType::RaftTransferLeader: {
              std::shared_ptr<raft_messagepb::TransferLeaderRequest>
                  tranLeader =
                      std::make_shared<raft_messagepb::TransferLeaderRequest>();
              tranLeader->ParseFromString(raftMsg->data());
              SPDLOG_INFO("transfer leader with peer id = " +
                          std::to_string(tranLeader->peer().id()));
              this->peer_->GetRaftGroup()->TransferLeader(
                  tranLeader->peer().id());
              break;
            }
            case raft_messagepb::RaftMessageType::RaftConfChange: {
              std::shared_ptr<raft_messagepb::ChangePeerRequest> peerChange =
                  std::make_shared<raft_messagepb::ChangePeerRequest>();
              peerChange->ParseFromString(raftMsg->data());
              {
                eraftpb::ConfChange confChange;
                confChange.set_node_id(peerChange->peer().id());
                confChange.set_context(peerChange->peer().addr());
                confChange.set_change_type(peerChange->change_type());
                this->peer_->GetRaftGroup()->ProposeConfChange(confChange);
              }
              break;
            }
            case raft_messagepb::RaftMessageType::RaftSplitRegion: {
              // std::shared_ptr<raft_cmdpb::SplitRequest> splitregion =
              //     std::make_shared<raft_cmdpb::SplitRequest>();
              // splitregion->ParseFromString(raftMsg->data());
              // {
              //   metapb::Region newregion;
              //   newregion.set_start_key(splitregion->split_key());
              //   newregion.set_id(splitregion->new_region_id());
              //   this->peer_->GetRaftGroup()->ProposeSplitRegion(newregion);
              // }
              break;
            }
          }
          delete raftMsg;
        } catch (const std::exception &e) {
          std::cerr << e.what() << '\n';
          SPDLOG_ERROR("handle with bad case!");
        }
        break;
      }
    }
    case MsgType::MsgTypeTick: {
      this->OnTick();
      break;
    }
    case MsgType::MsgTypeSplitRegion: {
      break;
    }
    case MsgType::MsgTypeRegionApproximateSize: {
      break;
    }
    case MsgType::MsgTypeGcSnap: {
      break;
    }
    case MsgType::MsgTypeStart: {
      this->StartTicker();
      break;
    }
    default:
      break;
  }
}

void RaftPeerMsgHandler::ProposeRequest(std::string palyload) {
  this->peer_->GetRaftGroup()->Propose(palyload);
}

void RaftPeerMsgHandler::ProposeRaftCommand(std::string playload) {
  // SPDLOG_INFO("PROPOSE TEST CMD ================= " + playload);
  // std::string data = put->key();
  this->peer_->GetRaftGroup()->Propose(playload);
}

void RaftPeerMsgHandler::OnTick() {
  SPDLOG_INFO("NODE STATE:" +
              eraft::StateToString(this->peer_->GetRaftGroup()->raft->state_));

  if (this->peer_->stopped_) {
    return;
  }

  this->OnRaftBaseTick();
  QueueContext::GetInstance()->get_regionIdCh().enqueue(this->peer_->regionId_);
}

void RaftPeerMsgHandler::StartTicker() {
  SPDLOG_INFO("start ticker!");
  QueueContext::GetInstance()->get_regionIdCh().enqueue(this->peer_->regionId_);
}

void RaftPeerMsgHandler::OnRaftBaseTick() {
  this->peer_->GetRaftGroup()->Tick();
}

bool RaftPeerMsgHandler::OnRaftMsg(raft_messagepb::RaftMessage *msg) {
  auto toPeer = this->peer_->GetPeerFromCache(msg->message().from());
  // peer is delete, do not handle
  if (toPeer == nullptr) {
    return false;
  }
  SPDLOG_INFO("on raft msg from " + std::to_string(msg->message().from()) +
              " to " + std::to_string(msg->message().to()) + " index " +
              std::to_string(msg->message().index()) + " term " +
              std::to_string(msg->message().term()) + " type " +
              eraft::MsgTypeToString(msg->message().msg_type()) +
              " ents.size " + std::to_string(msg->message().entries_size()));

  if (!msg->has_message()) {
    SPDLOG_INFO("nil message");
    return false;
  }

  if (this->peer_->stopped_) {
    return false;
  }
  eraftpb::Message newMsg;
  newMsg.set_from(msg->message().from());
  newMsg.set_to(msg->message().to());
  newMsg.set_index(msg->message().index());
  newMsg.set_term(msg->message().term());
  newMsg.set_commit(msg->message().commit());
  newMsg.set_log_term(msg->message().log_term());
  newMsg.set_reject(msg->message().reject());
  newMsg.set_msg_type(msg->message().msg_type());
  for (auto e : msg->message().entries()) {
    eraftpb::Entry *newE = newMsg.add_entries();
    newE->set_index(e.index());
    newE->set_data(e.data());
    newE->set_entry_type(e.entry_type());
    newE->set_term(e.term());
  }

  this->peer_->GetRaftGroup()->Step(newMsg);

  return true;
}

bool RaftPeerMsgHandler::CheckMessage(raft_messagepb::RaftMessage *msg) {
  return false;
}

}  // namespace network
