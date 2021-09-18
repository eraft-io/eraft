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

#include <Kv/bootstrap.h>
#include <Kv/concurrency_queue.h>
#include <Kv/peer.h>
#include <Kv/peer_msg_handler.h>
#include <Kv/raft_store.h>
#include <Kv/server.h>
#include <Kv/utils.h>
#include <RaftCore/raw_node.h>
#include <RaftCore/util.h>
#include <spdlog/spdlog.h>

#include <cassert>

namespace kvserver {

PeerMsgHandler::PeerMsgHandler(std::shared_ptr<Peer> peer,
                               std::shared_ptr<GlobalContext> ctx)
    : peer_(peer), ctx_(ctx) {}

PeerMsgHandler::~PeerMsgHandler() {}

std::shared_ptr<std::string> PeerMsgHandler::GetRequestKey(
    raft_cmdpb::Request* req) {
  std::string key;
  switch (req->cmd_type()) {
    case raft_cmdpb::CmdType::Get: {
      key = req->get().key();
      break;
    }
    case raft_cmdpb::CmdType::Put: {
      key = req->put().key();
      break;
    }
    case raft_cmdpb::CmdType::Delete: {
      key = req->delete_().key();
      break;
    }
    default:

      break;
  }
  return std::make_shared<std::string>(key);
}

void PeerMsgHandler::Handle(Proposal* p) {}

std::shared_ptr<rocksdb::WriteBatch> PeerMsgHandler::ProcessRequest(
    eraftpb::Entry* entry, raft_cmdpb::RaftCmdRequest* msg,
    std::shared_ptr<rocksdb::WriteBatch> wb) {
  auto req = msg->requests()[0];
  std::shared_ptr<std::string> key = GetRequestKey(&req);
  SPDLOG_INFO("process request key: " + *key);
  switch (req.cmd_type()) {
    case raft_cmdpb::CmdType::Get: {
      break;
    }
    case raft_cmdpb::CmdType::Put: {
      wb->Put(
          Assistant::GetInstance()->KeyWithCF(req.put().cf(), req.put().key()),
          req.put().value());
      SPDLOG_INFO("process put request with cf " + req.put().cf() + " key " +
                  req.put().key() + " value " + req.put().value());
      break;
    }
    case raft_cmdpb::CmdType::Delete: {
      wb->Delete(Assistant::GetInstance()->KeyWithCF(req.delete_().cf(),
                                                     req.delete_().key()));
      SPDLOG_INFO("process delete request with cf " + req.delete_().cf() +
                  " key " + req.delete_().key());
      break;
    }
    case raft_cmdpb::CmdType::Snap: {
      break;
    }
    default:
      break;
  }

  return wb;
}

void PeerMsgHandler::ProcessAdminRequest(
    eraftpb::Entry* entry, raft_cmdpb::RaftCmdRequest* req,
    std::shared_ptr<rocksdb::WriteBatch> wb) {}

void PeerMsgHandler::ProcessConfChange(
    eraftpb::Entry* entry, eraftpb::ConfChange* cc,
    std::shared_ptr<rocksdb::WriteBatch> wb) {
  switch (cc->change_type()) {
    case eraftpb::ConfChangeType::AddNode: {
      SPDLOG_INFO("add node");
      metapb::Peer* peer = new metapb::Peer();
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
  this->peer_->raftGroup_->ApplyConfChange(*cc);
}

void PeerMsgHandler::ProcessSplitRegion(
    eraftpb::Entry* entry, metapb::Region* newregion,
    std::shared_ptr<rocksdb::WriteBatch> wb) {
  uint64_t regionid = this->peer_->regionId_;
  std::unique_lock<std::mutex> mlock(this->ctx_->storeMeta_->mutex_);
  if (this->ctx_->storeMeta_->regions_.find(regionid) ==
      this->ctx_->storeMeta_->regions_.end()) {
    SPDLOG_INFO("ProcessSplitRegion() error not find");
    return;
  }
  metapb::Region* currentregion = this->ctx_->storeMeta_->regions_[regionid];
  metapb::RegionEpoch* rep = currentregion->mutable_region_epoch();
  rep->set_version(rep->version() + 1);
  for (auto per : currentregion->peers()) {
    metapb::Peer* newpeer = newregion->add_peers();
    newpeer->set_addr(per.addr());
    newpeer->set_store_id(per.store_id());
    newpeer->set_id(per.id());
  }
  newregion->mutable_region_epoch()->set_conf_ver(1);
  newregion->mutable_region_epoch()->set_version(1);
  newregion->set_end_key(currentregion->end_key());
  currentregion->set_end_key(newregion->start_key());
  this->ctx_->storeMeta_->regions_[newregion->id()] = newregion;
  mlock.unlock();
  std::string debugVal;
  google::protobuf::TextFormat::PrintToString(*currentregion, &debugVal);
  SPDLOG_INFO("oldregion: " + debugVal);
  google::protobuf::TextFormat::PrintToString(*newregion, &debugVal);
  SPDLOG_INFO("newregion: " + debugVal);
  metapb::Region curegion = *currentregion;
  metapb::Region neregion = *newregion;
  Assistant::GetInstance()->WriteRegionState(
      wb, std::make_shared<metapb::Region>(curegion),
      raft_serverpb::PeerState::Normal);
  Assistant::GetInstance()->WriteRegionState(
      wb, std::make_shared<metapb::Region>(neregion),
      raft_serverpb::PeerState::Normal);
  BootHelper::GetInstance()->PrepareBoostrapCluster(
      this->ctx_->engine_, std::make_shared<metapb::Region>(neregion));
  // this->peer_->sizeDiffHint_ = 0;
  // this->peer_->approximateSize_ = 0;
  std::shared_ptr<Peer> newpeer = std::make_shared<Peer>(
      this->ctx_->store_->id(), this->ctx_->cfg_, this->ctx_->engine_,
      std::make_shared<metapb::Region>(neregion));
  this->ctx_->router_->Register(newpeer);
  Msg m(MsgType::MsgTypeStart, neregion.id(), nullptr);
  this->ctx_->router_->Send(newregion->id(), m);
  SPDLOG_INFO("ProcessSplitRegion() before send");
}

std::shared_ptr<rocksdb::WriteBatch> PeerMsgHandler::Process(
    eraftpb::Entry* entry, std::shared_ptr<rocksdb::WriteBatch> wb) {
  switch (entry->entry_type()) {
    case eraftpb::EntryType::EntryConfChange: {
      SPDLOG_INFO("process conf change entry");
      eraftpb::ConfChange* cc = new eraftpb::ConfChange();
      cc->ParseFromString(entry->data());
      this->ProcessConfChange(entry, cc, wb);
      delete cc;
      break;
    }
    case eraftpb::EntryType::EntryNormal: {
      kvrpcpb::RawPutRequest* msg = new kvrpcpb::RawPutRequest();
      msg->ParseFromString(entry->data());
      SPDLOG_INFO("Process Entry DATA: cf " + msg->cf() + " key " + msg->key() +
                  " val " + msg->value());
      if (msg->cf().empty() && msg->key().empty()) {
        delete msg;
        return wb;
      }

      rocksdb::WriteBatch kvWB;
      if(msg->type() == 1){
          kvWB.Put(Assistant().GetInstance()->KeyWithCF(msg->cf(), msg->key()),
                    msg->value());   
      }else if(msg->type() == 2){
          kvWB.Delete(Assistant().GetInstance()->KeyWithCF(msg->cf(), msg->key()));
      }

      auto status = this->peer_->peerStorage_->engines_->kvDB_->Write(rocksdb::WriteOptions(),
                                                        &kvWB);
      if (!status.ok()) {
          SPDLOG_INFO("err: when process entry data() cf " + msg->cf() + " key " +
                      msg->key() + " val " + msg->value() + ")");
      }
      //  rocksdb::WriteBatch kvWB;
      // // write to kv db
      // auto status =
      //     kvWB.Put(Assistant().GetInstance()->KeyWithCF(msg->cf(), msg->key()),
      //              msg->value());
      // if (!status.ok()) {
      //   SPDLOG_INFO("err: when process entry data() cf " + msg->cf() + " key " +
      //               msg->key() + " val " + msg->value() + ")");
      // }
      // this->peer_->peerStorage_->engines_->kvDB_->Write(rocksdb::WriteOptions(),
      //                                                   &kvWB);

      if (this->peer_->IsLeader()) {
        std::mutex mapMutex;
        {
          std::lock_guard<std::mutex> lg(mapMutex);
          Server::readyCondVars_[msg->id()]
              ->notify_one();  // notify client process ok
        }
      }
      delete msg;
      break;
    }
    case eraftpb::EntryType::EntrySplitRegion: {
      metapb::Region* newregion = new metapb::Region();
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

void PeerMsgHandler::HandleRaftReady() {
  SPDLOG_INFO("handle raft ready " + std::to_string(this->peer_->regionId_));
  if (this->peer_->stopped_) {
    return;
  }
  if (this->peer_->raftGroup_->HasReady()) {
    SPDLOG_INFO("raft group is ready!");
    eraft::DReady rd = this->peer_->raftGroup_->EReady();
    auto result = this->peer_->peerStorage_->SaveReadyState(
        std::make_shared<eraft::DReady>(rd));
    if (result != nullptr) {
      // TODO
    }
    // real send raft message to transport (grpc)
    this->peer_->Send(this->ctx_->trans_, rd.messages);
    if (rd.committedEntries.size() > 0) {
      SPDLOG_INFO("rd.committedEntries.size() " +
                  std::to_string(rd.committedEntries.size()));
      std::shared_ptr<rocksdb::WriteBatch> kvWB =
          std::make_shared<rocksdb::WriteBatch>();
      for (auto entry : rd.committedEntries) {
        SPDLOG_INFO("COMMIT_ENTRY" + eraft::EntryToString(entry));
        kvWB = this->Process(&entry, kvWB);
        if (this->peer_->stopped_) {
          return;
        }
      }
      auto lastEnt = rd.committedEntries[rd.committedEntries.size() - 1];
      this->peer_->peerStorage_->applyState_->set_applied_index(
          lastEnt.index());
      this->peer_->peerStorage_->applyState_->mutable_truncated_state()
          ->set_index(lastEnt.index());
      this->peer_->peerStorage_->applyState_->mutable_truncated_state()
          ->set_term(lastEnt.term());
      SPDLOG_INFO(
          "write to db applied index: " + std::to_string(lastEnt.index()) +
          " truncated state index: " + std::to_string(lastEnt.index()) +
          " truncated state term " + std::to_string(lastEnt.term()));
      Assistant::GetInstance()->SetMeta(
          kvWB.get(),
          Assistant::GetInstance()->ApplyStateKey(this->peer_->regionId_),
          *this->peer_->peerStorage_->applyState_);
      this->peer_->peerStorage_->engines_->kvDB_->Write(rocksdb::WriteOptions(),
                                                        kvWB.get());
    }
    this->peer_->raftGroup_->Advance(rd);
  }
}

void PeerMsgHandler::HandleMsg(Msg m) {
  SPDLOG_INFO("handle raft msg type " + m.MsgToString());
  switch (m.type_) {
    case MsgType::MsgTypeRaftMessage: {
      if (m.data_ != nullptr) {
        try {
          auto raftMsg = static_cast<raft_serverpb::RaftMessage*>(m.data_);
          if (raftMsg == nullptr) {
            SPDLOG_ERROR("handle nil message");
            return;
          }
          switch (raftMsg->raft_msg_type()) {
            case raft_serverpb::RaftMsgNormal: {
              if (!this->OnRaftMsg(raftMsg)) {
                SPDLOG_ERROR("on handle raft msg");
              }
              break;
            }
            case raft_serverpb::RaftMsgClientCmd: {
              std::shared_ptr<kvrpcpb::RawPutRequest> put =
                  std::make_shared<kvrpcpb::RawPutRequest>();
              put->set_key(raftMsg->data());
              SPDLOG_INFO("PROPOSE NEW: " + put->key());
              this->ProposeRaftCommand(put.get());
              break;
            }
            case raft_serverpb::RaftTransferLeader: {
              std::shared_ptr<raft_cmdpb::TransferLeaderRequest> tranLeader =
                  std::make_shared<raft_cmdpb::TransferLeaderRequest>();
              tranLeader->ParseFromString(raftMsg->data());
              SPDLOG_INFO("transfer leader with peer id = " +
                          std::to_string(tranLeader->peer().id()));
              this->peer_->raftGroup_->TransferLeader(tranLeader->peer().id());
              break;
            }
            case raft_serverpb::RaftConfChange: {
              std::shared_ptr<raft_cmdpb::ChangePeerRequest> peerChange =
                  std::make_shared<raft_cmdpb::ChangePeerRequest>();
              peerChange->ParseFromString(raftMsg->data());
              {
                eraftpb::ConfChange confChange;
                confChange.set_node_id(peerChange->peer().id());
                confChange.set_context(peerChange->peer().addr());
                confChange.set_change_type(peerChange->change_type());
                this->peer_->raftGroup_->ProposeConfChange(confChange);
              }
              break;
            }
            case raft_serverpb::RaftSplitRegion: {
              std::shared_ptr<raft_cmdpb::SplitRequest> splitregion =
                  std::make_shared<raft_cmdpb::SplitRequest>();
              splitregion->ParseFromString(raftMsg->data());
              {
                metapb::Region newregion;
                newregion.set_start_key(splitregion->split_key());
                newregion.set_id(splitregion->new_region_id());
                this->peer_->raftGroup_->ProposeSplitRegion(newregion);
              }
              break;
            }
          }
          delete raftMsg;
        } catch (const std::exception& e) {
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
  }  // namespace kvserver
}

bool PeerMsgHandler::CheckStoreID(raft_cmdpb::RaftCmdRequest* req,
                                  uint64_t storeID) {
  auto peer = req->header().peer();
  if (peer.store_id() == storeID) {
    return true;
  }
  return false;
}

bool PeerMsgHandler::PreProposeRaftCommand(raft_cmdpb::RaftCmdRequest* req) {
  if (!this->CheckStoreID(req, this->peer_->meta_->store_id())) {
    // check store id, make sure that msg is dispatched to the right
    // place
    return false;
  }
  auto regionID = this->peer_->regionId_;
  auto leaderID = this->peer_->LeaderId();
  if (!this->peer_->IsLeader()) {
    auto leader = this->peer_->GetPeerFromCache(leaderID);
    // log err not leader
    return false;
  }
  if (!this->CheckPeerID(req, this->peer_->PeerId())) {
    // peer_id must be the same as peer's
    return false;
  }
  // check whether the term is stale.
  if (!this->CheckTerm(req, this->peer_->Term())) {
    return false;
  }
  return true;
}

bool PeerMsgHandler::CheckPeerID(raft_cmdpb::RaftCmdRequest* req,
                                 uint64_t peerID) {
  auto peer = req->header().peer();
  if (peer.id() == peerID) {
    return true;
  }
  return false;
}

bool PeerMsgHandler::CheckTerm(raft_cmdpb::RaftCmdRequest* req, uint64_t term) {
  auto header = req->header();
  if (header.term() == 0 || term <= header.term() + 1) {
    return true;
  }
  return false;
}

void PeerMsgHandler::ProposeAdminRequest(raft_cmdpb::RaftCmdRequest* msg,
                                         Callback* cb) {}

void PeerMsgHandler::ProposeRequest(kvrpcpb::RawPutRequest* put) {
  SPDLOG_INFO("PROPOSE TEST KEY ================= " + put->key());
  std::string data = put->key();
  this->peer_->raftGroup_->Propose(data);
}

void PeerMsgHandler::ProposeRaftCommand(kvrpcpb::RawPutRequest* put) {
  // TODO: check put
  this->ProposeRequest(put);
}

void PeerMsgHandler::OnTick() {
  SPDLOG_INFO("NODE STATE:" +
              eraft::StateToString(this->peer_->raftGroup_->raft->state_));

  if (this->peer_->stopped_) {
    return;
  }

  this->OnRaftBaseTick();
  QueueContext::GetInstance()->get_regionIdCh().Push(this->peer_->regionId_);
}

void PeerMsgHandler::StartTicker() {
  SPDLOG_INFO("start ticker!");
  QueueContext::GetInstance()->get_regionIdCh().Push(this->peer_->regionId_);
}

void PeerMsgHandler::OnRaftBaseTick() { this->peer_->raftGroup_->Tick(); }

bool PeerMsgHandler::OnRaftMsg(raft_serverpb::RaftMessage* msg) {
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
    eraftpb::Entry* newE = newMsg.add_entries();
    newE->set_index(e.index());
    newE->set_data(e.data());
    newE->set_entry_type(e.entry_type());
    newE->set_term(e.term());
  }

  this->peer_->raftGroup_->Step(newMsg);

  return true;
}

bool PeerMsgHandler::ValidateRaftMessage(raft_serverpb::RaftMessage* msg) {
  auto to = msg->to_peer();

  if (to.store_id() != this->peer_->storeID()) {
    return false;
  }

  return true;
}

bool PeerMsgHandler::CheckMessage(raft_serverpb::RaftMessage* msg) {
  auto fromEpoch = msg->region_epoch();
  auto isVoteMsg =
      (msg->message().msg_type() == eraftpb::MessageType::MsgRequestVote);
  auto fromStoreID = msg->from_peer().store_id();

  auto region = this->peer_->Region();
  auto target = msg->to_peer();

  if (target.id() < this->peer_->PeerId()) {
    return true;
  } else if (target.id() > this->peer_->PeerId()) {
    return true;
  }

  return false;
}

}  // namespace kvserver
