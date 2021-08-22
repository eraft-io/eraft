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

#include <Kv/concurrency_queue.h>
#include <Kv/peer_msg_handler.h>
#include <Kv/utils.h>
#include <Logger/logger.h>
#include <RaftCore/raw_node.h>
#include <RaftCore/util.h>

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

//
// current not used method
//
std::shared_ptr<rocksdb::WriteBatch> PeerMsgHandler::ProcessRequest(
    eraftpb::Entry* entry, raft_cmdpb::RaftCmdRequest* msg,
    std::shared_ptr<rocksdb::WriteBatch> wb) {
  auto req = msg->requests()[0];
  std::shared_ptr<std::string> key = GetRequestKey(&req);
  Logger::GetInstance()->DEBUG_NEW("process request key: " + *key, __FILE__,
                                   __LINE__, "PeerMsgHandler::ProcessRequest");
  switch (req.cmd_type()) {
    case raft_cmdpb::CmdType::Get: {
      break;
    }
    case raft_cmdpb::CmdType::Put: {
      wb->Put(
          Assistant::GetInstance()->KeyWithCF(req.put().cf(), req.put().key()),
          req.put().value());
      Logger::GetInstance()->DEBUG_NEW(
          "process put request with cf " + req.put().cf() + " key " +
              req.put().key() + " value " + req.put().value(),
          __FILE__, __LINE__, "PeerMsgHandler::ProcessRequest");
      break;
    }
    case raft_cmdpb::CmdType::Delete: {
      wb->Delete(Assistant::GetInstance()->KeyWithCF(req.delete_().cf(),
                                                     req.delete_().key()));
      Logger::GetInstance()->DEBUG_NEW(
          "process delete request with cf " + req.delete_().cf() + " key " +
              req.delete_().key(),
          __FILE__, __LINE__, "PeerMsgHandler::ProcessRequest");
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
    eraftpb::Entry entry, std::unique_ptr<eraftpb::ConfChange> cc,
    std::shared_ptr<rocksdb::WriteBatch> wb) {
  switch (cc->change_type()) {
    case eraftpb::ConfChangeType::AddNode: {
      Logger::GetInstance()->DEBUG_NEW("add node", __FILE__, __LINE__,
                                       "PeerMsgHandler::ProcessConfChange");
      std::shared_ptr<metapb::Peer> peer = std::make_shared<metapb::Peer>();
      peer->set_id(cc->node_id());
      peer->set_store_id(cc->node_id());
      peer->set_addr(cc->context());
      this->peer_->InsertPeerCache(peer);
      break;
    }
    case eraftpb::ConfChangeType::RemoveNode: {
      Logger::GetInstance()->DEBUG_NEW("remove node", __FILE__, __LINE__,
                                       "PeerMsgHandler::ProcessConfChange");
      this->peer_->RemovePeerCache(cc->node_id());
      break;
    }
    default:
      break;
  }
  this->peer_->raftGroup_->ApplyConfChange(*cc);
}

//
// process the commit entry, return write batch
//

std::shared_ptr<rocksdb::WriteBatch> PeerMsgHandler::Process(
    eraftpb::Entry entry, std::shared_ptr<rocksdb::WriteBatch> wb) {
  switch (entry.entry_type()) {
    case eraftpb::EntryType::EntryConfChange: {
      Logger::GetInstance()->DEBUG_NEW(
          "add", __FILE__, __LINE__, "PeerMsgHandler::Proces EntryConfChanges");
      std::unique_ptr<eraftpb::ConfChange> cc(new eraftpb::ConfChange);
      cc->ParseFromString(entry.data());
      this->ProcessConfChange(entry, std::move(cc), wb);
      break;
    }
    case eraftpb::EntryType::EntryNormal: {
      std::unique_ptr<kvrpcpb::RawPutRequest> msg(new kvrpcpb::RawPutRequest);
      msg->ParseFromString(entry.data());
      Logger::GetInstance()->DEBUG_NEW(
          "Process Entry DATA: cf " + msg->cf() + " key " + msg->key() +
              " val " + msg->value(),
          __FILE__, __LINE__, "eerMsgHandler::Process");
      // write to kv db
      auto status =
          wb->Put(Assistant().GetInstance()->KeyWithCF(msg->cf(), msg->key()),
                  msg->value());
      if (!status.ok()) {
        Logger::GetInstance()->DEBUG_NEW(
            "err: when process entry data() cf " + msg->cf() + " key " +
                msg->key() + " val " + msg->value() + ")",
            __FILE__, __LINE__, "eerMsgHandler::Process");
      }
      break;
    }
    default:
      break;
  }
  return wb;
}

//
// handle raft ready
// check if raft group is ready now
// save current ready state
// write commit entry to state machine
// save apply state
//
void PeerMsgHandler::HandleRaftReady() {
  Logger::GetInstance()->DEBUG_NEW("handle raft ready ", __FILE__, __LINE__,
                                   "PeerMsgHandler::HandleRaftReady");
  if (this->peer_->stopped_) {
    return;
  }
  if (this->peer_->raftGroup_->HasReady()) {
    Logger::GetInstance()->DEBUG_NEW("raft group is ready!", __FILE__, __LINE__,
                                     "PeerMsgHandler::HandleRaftReady");
    eraft::DReady rd = this->peer_->raftGroup_->EReady();
    auto result = this->peer_->peerStorage_->SaveReadyState(
        std::make_shared<eraft::DReady>(rd));
    if (result != nullptr) {
      // TODO
    }
    // real send raft message to transport (grpc)
    this->peer_->Send(this->ctx_->trans_, rd.messages);
    if (rd.committedEntries.size() > 0) {
      Logger::GetInstance()->DEBUG_NEW(
          "rd.committedEntries.size() " +
              std::to_string(rd.committedEntries.size()),
          __FILE__, __LINE__, "PeerMsgHandler::HandleRaftReady");
      std::shared_ptr<rocksdb::WriteBatch> kvWB =
          std::make_shared<rocksdb::WriteBatch>();
      for (auto entry : rd.committedEntries) {
        Logger::GetInstance()->DEBUG_NEW(
            "COMMIT_ENTRY" + eraft::EntryToString(entry), __FILE__, __LINE__,
            "PeerMsgHandler::HandleRaftReady");
        kvWB = this->Process(entry, kvWB);
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
      Logger::GetInstance()->DEBUG_NEW(
          "write to db applied index: " + std::to_string(lastEnt.index()) +
              " truncated state index: " + std::to_string(lastEnt.index()) +
              " truncated state term " + std::to_string(lastEnt.term()),
          __FILE__, __LINE__, "PeerMsgHandler::HandleRaftReady");
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

//
// handle all the message
// MsgTypeRaftMessage::RaftMsgNormal: raft group inner msg
// MsgTypeRaftMessage::RaftMsgClientCmd: the msg from client node
// MsgTypeRaftMessage::RaftTransferLeader: the msg for transfer group leader
// MsgTypeRaftMessage::RaftConfChange: the msg for change the group node conf
// MsgTypeTick: the tick msg for raft group system
// MsgTypeStart: the msg to trigger ticker start
//
void PeerMsgHandler::HandleMsg(Msg m) {
  Logger::GetInstance()->DEBUG_NEW("handle raft msg type " + m.MsgToString(),
                                   __FILE__, __LINE__,
                                   "PeerMsgHandler::HandleMsg");
  switch (m.type_) {
    case MsgType::MsgTypeRaftMessage: {
      if (m.data_ != nullptr) {
        try {
          auto raftMsg = static_cast<raft_serverpb::RaftMessage*>(m.data_);
          if (raftMsg == nullptr) {
            Logger::GetInstance()->DEBUG_NEW("err: nil message", __FILE__,
                                             __LINE__,
                                             "PeerMsgHandler::HandleMsg");
            return;
          }
          switch (raftMsg->raft_msg_type()) {
            case raft_serverpb::RaftMsgNormal: {
              if (!this->OnRaftMsg(raftMsg)) {
                Logger::GetInstance()->DEBUG_NEW("err: on handle raft msg",
                                                 __FILE__, __LINE__,
                                                 "PeerMsgHandler::HandleMsg");
              }
              break;
            }
            case raft_serverpb::RaftMsgClientCmd: {
              std::shared_ptr<kvrpcpb::RawPutRequest> put =
                  std::make_shared<kvrpcpb::RawPutRequest>();
              put->set_key(raftMsg->data());  // data
              Logger::GetInstance()->DEBUG_NEW("PROPOSE NEW: " + put->key(),
                                               __FILE__, __LINE__,
                                               "PeerMsgHandler::HandleMsg");
              this->ProposeRaftCommand(put.get());
              break;
            }
            case raft_serverpb::RaftTransferLeader: {
              std::shared_ptr<raft_cmdpb::TransferLeaderRequest> tranLeader =
                  std::make_shared<raft_cmdpb::TransferLeaderRequest>();
              tranLeader->ParseFromString(raftMsg->data());
              Logger::GetInstance()->DEBUG_NEW(
                  "transfer leader with peer id = " +
                      std::to_string(tranLeader->peer().id()),
                  __FILE__, __LINE__, "PeerMsgHandler::HandleMsg");
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
          }
        } catch (const std::exception& e) {
          std::cerr << e.what() << '\n';
          // TODO:
          Logger::GetInstance()->DEBUG_NEW("BAD CASE", __FILE__, __LINE__,
                                           "PeerMsgHandler::HandleMsg");
          // Try again
        }
        break;
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
    // check store id, make sure that msg is dispatched to the right place
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
  Logger::GetInstance()->DEBUG_NEW(
      "PROPOSE TEST KEY ================= " + put->key(), __FILE__, __LINE__,
      "PeerMsgHandler::ProposeRequest");
  std::string data = put->key();
  this->peer_->raftGroup_->Propose(data);
}

void PeerMsgHandler::ProposeRaftCommand(kvrpcpb::RawPutRequest* put) {
  // TODO: check put
  this->ProposeRequest(put);
}

void PeerMsgHandler::OnTick() {
  Logger::GetInstance()->DEBUG_NEW(
      "NODE STATE:" +
          eraft::StateToString(this->peer_->raftGroup_->raft->state_),
      __FILE__, __LINE__, "PeerMsgHandler::OnTick");

  if (this->peer_->stopped_) {
    return;
  }

  this->OnRaftBaseTick();
  QueueContext::GetInstance()->get_regionIdCh().Push(this->peer_->regionId_);
}

void PeerMsgHandler::StartTicker() {
  Logger::GetInstance()->DEBUG_NEW("start ticker", __FILE__, __LINE__,
                                   "PeerMsgHandler::StartTicker");
  QueueContext::GetInstance()->get_regionIdCh().Push(this->peer_->regionId_);
}

void PeerMsgHandler::OnRaftBaseTick() { this->peer_->raftGroup_->Tick(); }

bool PeerMsgHandler::OnRaftMsg(raft_serverpb::RaftMessage* msg) {
  auto toPeer = this->peer_->GetPeerFromCache(msg->message().from());
  // peer is delete, do not handle
  if (toPeer == nullptr) {
    return false;
  }
  Logger::GetInstance()->DEBUG_NEW(
      "on raft msg from " + std::to_string(msg->message().from()) + " to " +
          std::to_string(msg->message().to()) + " index " +
          std::to_string(msg->message().index()) + " term " +
          std::to_string(msg->message().term()) + " type " +
          eraft::MsgTypeToString(msg->message().msg_type()) + " ents.size " +
          std::to_string(msg->message().entries_size()),
      __FILE__, __LINE__, "PeerMsgHandler::OnRaftMsg");

  if (!msg->has_message()) {
    Logger::GetInstance()->DEBUG_NEW("nil message", __FILE__, __LINE__,
                                     "PeerMsgHandler::OnRaftMsg");
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
  Logger::GetInstance()->DEBUG_NEW("RECIVED ENTRY DATA", __FILE__, __LINE__,
                                   "RaftContext::OnRaftMsg");
  for (auto e : msg->message().entries()) {
    eraftpb::Entry* newE = newMsg.add_entries();
    newE->set_index(e.index());
    newE->set_data(e.data());
    newE->set_entry_type(e.entry_type());
    newE->set_term(e.term());
  }

  this->peer_->raftGroup_->Step(std::move(newMsg));

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
