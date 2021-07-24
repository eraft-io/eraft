#include <Kv/peer_msg_handler.h>
#include <Kv/utils.h>
#include <RaftCore/RawNode.h>

#include <cassert>

namespace kvserver
{

PeerMsgHandler::PeerMsgHandler(std::shared_ptr<Peer> peer, std::shared_ptr<GlobalContext> ctx)
{
    this->peer_ = peer;
    this->ctx_ = ctx;   
}

PeerMsgHandler::~PeerMsgHandler()
{

}

void PeerMsgHandler::Read()
{

}

void PeerMsgHandler::HandleProposal(eraftpb::Entry* entry, std::function<void(Proposal*)> handle)
{
    if (this->peer_->proposals_.size() > 0)
    {
        Proposal p = this->peer_->proposals_[0];
        if(p.index_ == entry->index())
        {
            if(p.term_ != entry->term())
            {
                // TODO: notify stale req
            } 
            else 
            {
                handle(&p);
            }
        }
        this->peer_->proposals_.erase(this->peer_->proposals_.begin());
    }
}
 
std::shared_ptr<std::string> PeerMsgHandler::GetRequestKey(raft_cmdpb::Request *req)
{
    std::string key;
    switch (req->cmd_type())
    {
    case raft_cmdpb::CmdType::Get:
        {
            key = req->get().key();
            break;
        }
    case raft_cmdpb::CmdType::Put:
        {
            key = req->put().key();
            break;
        }
    case raft_cmdpb::CmdType::Delete:
        {
            key = req->delete_().key();
            break;
        }
    default:
        break;
    }
    return std::make_shared<std::string>(key);
}

void PeerMsgHandler::Handle(Proposal* p)
{
    
}


std::shared_ptr<rocksdb::WriteBatch> PeerMsgHandler::ProcessRequest(eraftpb::Entry* entry, raft_cmdpb::RaftCmdRequest* msg, std::shared_ptr<rocksdb::WriteBatch> wb)
{
    auto req = msg->requests()[0];
    std::shared_ptr<std::string> key = GetRequestKey(&req);
    if(key != nullptr) 
    {
        // TODO: check key in region
        // this->HandleProposal(entry, );
    }
    switch (req.cmd_type())
    {
    case raft_cmdpb::CmdType::Get:
        {
            break;
        }
    case raft_cmdpb::CmdType::Put:
        {
            wb->Put(Assistant::GetInstance()->KeyWithCF(req.put().cf(), req.put().key()), req.put().value());
            break;
        }
    case raft_cmdpb::CmdType::Delete:
        {
            wb->Delete(Assistant::GetInstance()->KeyWithCF(req.delete_().cf(), req.delete_().key()));
            break;
        }
    case raft_cmdpb::CmdType::Snap:
        {
            break;
        }
    default:
        break;
    }

    //
    // [capture list] (params list) mutable exception-> return type { function body }
    // 各项具体含义如下
    // capture list：捕获外部变量列表
    // params list：形参列表
    // mutable指示符：用来说用是否可以修改捕获的变量
    // exception：异常设定
    // return type：返回类型
    // function body：函数体
    //

    this->HandleProposal(
        entry,
        [&](Proposal* p) { // 以引用形式捕获所有外部变量
            raft_cmdpb::RaftCmdResponse* resp = new raft_cmdpb::RaftCmdResponse();
            switch (req.cmd_type())
            {
            case raft_cmdpb::CmdType::Get:
                {
                    // update apply state to db
                    this->peer_->peerStorage_->applyState_->set_applied_index(entry->index());
                    std::string applyKey(Assistant::GetInstance()->ApplyStateKey(this->peer_->regionId_).begin(), 
                    Assistant::GetInstance()->ApplyStateKey(this->peer_->regionId_).end());
                    Assistant::GetInstance()->SetMeta(wb.get(), applyKey, *this->peer_->peerStorage_->applyState_);
                    this->peer_->peerStorage_->engines_->kvDB_->Write(rocksdb::WriteOptions(),& *wb);
                    // get value from db
                    std::string value = Assistant::GetInstance()->GetCF(this->peer_->peerStorage_->engines_->kvDB_, req.get().cf(), req.get().key());
                    raft_cmdpb::Response* sp = resp->add_responses();                    
                    sp->set_cmd_type(raft_cmdpb::CmdType::Get);
                    raft_cmdpb::GetResponse* gsp = new raft_cmdpb::GetResponse();
                    gsp->set_value(value);
                    sp->set_allocated_get(gsp);
                    break;
                }
            case raft_cmdpb::CmdType::Put:
                {
                    raft_cmdpb::Response* sp = resp->add_responses();
                    sp->set_cmd_type(raft_cmdpb::CmdType::Put);
                    raft_cmdpb::PutResponse* psp = new raft_cmdpb::PutResponse();
                    sp->set_allocated_put(psp);
                    break;
                }
            case raft_cmdpb::CmdType::Delete:
                {
                    raft_cmdpb::Response* sp = resp->add_responses();
                    sp->set_cmd_type(raft_cmdpb::CmdType::Delete);
                    raft_cmdpb::DeleteResponse* deleteResp = new raft_cmdpb::DeleteResponse();
                    sp->set_allocated_delete_(deleteResp);
                    break;   
                }
            case raft_cmdpb::CmdType::Snap:
                {
                    // TODO: snap
                    break;
                }
            default:
                break;
            }
            p->cb_->Done(resp);
        }
    );

    return wb;
}

void PeerMsgHandler::ProcessAdminRequest(eraftpb::Entry* entry, raft_cmdpb::RaftCmdRequest* req, std::shared_ptr<rocksdb::WriteBatch> wb)
{

}

size_t PeerMsgHandler::SearchRegionPeer(std::shared_ptr<metapb::Region> region, uint64_t id)
{
    size_t i = 0;
    for(auto peer: region->peers())
    {
        if(peer.id() == id) 
        {
            return i;
        }
        i++;
    }
    return region->peers().size();
}

void PeerMsgHandler::ProcessConfChange(eraftpb::Entry* entry, eraftpb::ConfChange* cc, std::shared_ptr<rocksdb::WriteBatch> wb)
{
    raft_cmdpb::RaftCmdRequest msg;
    msg.ParseFromString(cc->context());

    auto region = this->peer_->Region();
    // TODO: check region epoch

    switch (cc->change_type())
    {
    case eraftpb::ConfChangeType::AddNode:
        {
            auto n = this->SearchRegionPeer(region, cc->node_id());
            if (n == region->peers().size())
            {
                auto p = msg.admin_request().change_peer().peer();
                auto addPeer = region->add_peers();
                addPeer->set_id(p.id());
                addPeer->set_store_id(p.store_id());
                // incr conf ver
                uint64_t confVer = region->region_epoch().conf_ver();
                auto epoch = region->mutable_region_epoch();
                epoch->set_conf_ver(confVer++);
                // store region state
                Assistant::GetInstance()->WriteRegionState(wb, region, raft_serverpb::PeerState::Normal);
                auto storeMeta = this->ctx_->storeMeta_;
                storeMeta->mutex_.lock();
                storeMeta->regions_[region->id()] = & *region;
                storeMeta->mutex_.unlock();
                this->peer_->InsertPeerCache(addPeer);
            }
            break;
        }
    case eraftpb::ConfChangeType::RemoveNode:
        {
            if (cc->node_id() == this->peer_->meta_->id())
            {
                this->DestoryPeer();
                return;
            }
            size_t n = this->SearchRegionPeer(region, cc->node_id());
            if (n < region->peers().size())
            {
                // erase no n peer
                auto peers = region->mutable_peers();
                peers->erase(peers->begin() + n);
                // incr conf ver
                uint64_t confVer = region->region_epoch().conf_ver();
                auto epoch = region->mutable_region_epoch();
                epoch->set_conf_ver(confVer++);
                // store region state
                Assistant::GetInstance()->WriteRegionState(wb, region, raft_serverpb::PeerState::Normal);
                auto storeMeta = this->ctx_->storeMeta_;
                storeMeta->mutex_.lock();
                storeMeta->regions_[region->id()] = & *region;
                storeMeta->mutex_.unlock();
                this->peer_->RemovePeerCache(cc->node_id());
            }
            break;
        }
    default:
        break;
    }
    this->peer_->raftGroup_->ApplyConfChange(*cc);
    this->HandleProposal(entry, [&](Proposal* p) {
        raft_cmdpb::AdminResponse* adminResp = new raft_cmdpb::AdminResponse;
        adminResp->set_cmd_type(raft_cmdpb::AdminCmdType::ChangePeer);
        raft_cmdpb::RaftCmdResponse* raftCmdResp = new raft_cmdpb::RaftCmdResponse;
        raftCmdResp->set_allocated_admin_response(adminResp);
        p->cb_->Done(raftCmdResp);
    });

    // TODO: heartneat scheduler
}

std::shared_ptr<rocksdb::WriteBatch> PeerMsgHandler::Process(eraftpb::Entry* entry, std::shared_ptr<rocksdb::WriteBatch> wb)
{
    if (entry->entry_type() == eraftpb::EntryType::EntryConfChange)
    {
        eraftpb::ConfChange* cc = new eraftpb::ConfChange();
        cc->ParseFromString(entry->data());
        this->ProcessConfChange(entry, cc, wb);
        return wb;
    }
    raft_cmdpb::RaftCmdRequest* msg = new raft_cmdpb::RaftCmdRequest();
    msg->ParseFromString(entry->data());
    if (msg->requests().size() > 0)
    {
        this->ProcessRequest(entry, msg, wb);
    }
    if (msg->mutable_admin_request() != nullptr)
    {
        this->ProcessAdminRequest(entry, msg, wb);
        return wb;
    }

    // no op entry
    return wb;
}

void PeerMsgHandler::HandleRaftReady()
{
    if(this->peer_->stopped_) 
    {
        return;
    }
    if(this->peer_->raftGroup_->HasReady())
    {
        eraft::DReady rd = this->peer_->raftGroup_->EReady();
        auto result = this->peer_->peerStorage_->SaveReadyState(std::make_shared<eraft::DReady>(rd));
        if(result != nullptr)
        {
            // TODO: snapshot
        }
        this->peer_->Send(this->ctx_->trans_, rd.messages);
        if(rd.committedEntries.size() > 0)
        {
            std::vector<Proposal> oldProposal = this->peer_->proposals_;   
            std::shared_ptr<rocksdb::WriteBatch> kvWB = std::make_shared<rocksdb::WriteBatch>();
            for( auto entry: rd.committedEntries)
            {
                kvWB = this->Process(&entry, kvWB);
                if (this->peer_->stopped_)
                {
                    return;
                }
            }
            this->peer_->peerStorage_->applyState_->set_applied_index(rd.committedEntries[rd.committedEntries.size() - 1].index());
            std::string key(Assistant::GetInstance()->ApplyStateKey(this->peer_->regionId_).begin(), Assistant::GetInstance()->ApplyStateKey(this->peer_->regionId_).end());
            Assistant::GetInstance()->SetMeta(kvWB.get(), key, *this->peer_->peerStorage_->applyState_);
            this->peer_->peerStorage_->engines_->kvDB_->Write(rocksdb::WriteOptions(), kvWB.get());
            if (oldProposal.size() > this->peer_->proposals_.size())
            {
                // TODO:
            }
        }
        this->peer_->raftGroup_->Advance(rd);
    }
}

void PeerMsgHandler::HandleMsg(Msg m)
{
    switch (m.type_)
    {
    case MsgType::MsgTypeRaftMessage:
        {
            auto raftMsg = static_cast<raft_serverpb::RaftMessage*>(m.data_);
            if(!this->OnRaftMsg(raftMsg)) 
            {
                // TODO: log handle raft message error
            }
            break;
        }
    case MsgType::MsgTypeRaftCmd:
        {
            auto raftCmd = static_cast<MsgRaftCmd*>(m.data_);
            this->ProposeRaftCommand(raftCmd->request_, raftCmd->callback_);
            break;
        }
    case MsgType::MsgTypeTick:
        {
            this->OnTick();
            break;
        }
    case MsgType::MsgTypeSplitRegion:
        {
            // TODO: split region
            // auto split = static_cast<Msg>
            break;
        }
    case MsgType::MsgTypeRegionApproximateSize:
        {
            // TODO:
            break;
        }
    case MsgType::MsgTypeGcSnap:
        {
            // TODO:
            break;
        }
    case MsgType::MsgTypeStart:
        {
            this->StartTicker();
            break;
        }
    default:
        break;
    }
}

bool PeerMsgHandler::CheckStoreID(raft_cmdpb::RaftCmdRequest* req, uint64_t storeID)
{
    auto peer = req->header().peer();
    if(peer.store_id() == storeID) {
        return true;
    }
    return false; // store not match
}


bool PeerMsgHandler::PreProposeRaftCommand(raft_cmdpb::RaftCmdRequest* req)
{
    if(!this->CheckStoreID(req, this->peer_->meta_->store_id()))
    {
        // check store id, make sure that msg is dispatched to the right place
        return false;
    }
    auto regionID = this->peer_->regionId_;
    auto leaderID = this->peer_->LeaderId();
    if(!this->peer_->IsLeader())
    {
        auto leader = this->peer_->GetPeerFromCache(leaderID);
        // log err not leader
        return false;
    }
    if(!this->CheckPeerID(req, this->peer_->PeerId()))
    {
        // peer_id must be the same as peer's
        return false;
    }
    // check whether the term is stale.
    if(!this->CheckTerm(req, this->peer_->Term()))
    {
        return false;
    }
    return true; // no err
}

bool PeerMsgHandler::CheckPeerID(raft_cmdpb::RaftCmdRequest* req, uint64_t peerID)
{
    auto peer = req->header().peer();
    if(peer.id() == peerID)
    {
        return true;
    }
    return false;
}

bool PeerMsgHandler::CheckTerm(raft_cmdpb::RaftCmdRequest* req, uint64_t term)
{
    auto header = req->header();
    if(header.term() == 0 || term <= header.term() + 1)   
    {
        return true;
    }
    return false;
}


bool PeerMsgHandler::CheckKeyInRegion(std::string key, std::shared_ptr<metapb::Region> region)
{
    if(key.compare(region->start_key()) >= 0 && (region->end_key().size() == 0 || key.compare(region->end_key()) < 0))
    {
        return true;
    }
    else 
    {
        // key is not in range [`start_key`, `end_key`)
        return false;
    }
}




void PeerMsgHandler::ProposeAdminRequest(raft_cmdpb::RaftCmdRequest* msg, Callback* cb)
{

}

void PeerMsgHandler::ProposeRequest(raft_cmdpb::RaftCmdRequest* msg, Callback* cb)
{
    auto req = msg->requests()[0];
    std::shared_ptr<std::string> key = GetRequestKey(&req);
    if(*key != "")
    {
        assert(CheckKeyInRegion(*key, this->peer_->Region()));
    }
    std::string data = msg->SerializeAsString();
    // add proposal to peer
    Proposal p;
    p.index_ = this->peer_->NextProposalIndex();
    p.term_ = this->peer_->Term();
    p.cb_ = cb;
    this->peer_->proposals_.push_back(p);
    std::vector<uint8_t> prop_(data.begin(), data.end());
    this->peer_->raftGroup_->Propose(&prop_);
}

void PeerMsgHandler::ProposeRaftCommand(raft_cmdpb::RaftCmdRequest* msg, Callback* cb)
{
    assert(this->PreProposeRaftCommand(msg));

    // 
    // TODO: check if msg is admin req
    //
    // this->ProposeAdminRequest(msg, cb);
    // 
    this->ProposeRequest(msg, cb);
}

void PeerMsgHandler::OnTick()
{
    if(this->peer_->stopped_)
    {
        return;
    }
    //
    // TODO: 
    this->OnRaftBaseTick();
}

void PeerMsgHandler::StartTicker()
{
    // add task to ticker
    
}

void PeerMsgHandler::OnRaftBaseTick()
{
    this->peer_->raftGroup_->Tick();
}

void PeerMsgHandler::ScheduleCompactLog(uint64_t firstIndex, uint64_t truncatedIndex)
{
}

bool PeerMsgHandler::OnRaftMsg(raft_serverpb::RaftMessage* msg)
{
    if(!this->ValidateRaftMessage(msg))
    {
        return false;
    }
    if(this->peer_->stopped_)
    {
        return false;
    }
    if(msg->is_tombstone())
    {
        this->HandleGCPeerMsg(msg);
        return false;
    }
    // TODO: snap 
    this->peer_->InsertPeerCache(msg->mutable_from_peer());
    this->peer_->raftGroup_->Step(msg->message());
    if(this->peer_->AnyNewPeerCatchUp(msg->from_peer().id()))
    {
        // TODO:
    }
}

bool PeerMsgHandler::ValidateRaftMessage(raft_serverpb::RaftMessage* msg)
{
    auto regionID = msg->region_id();
    auto from = msg->from_peer();
    auto to = msg->to_peer();

    if(to.store_id() != this->peer_->storeID())
    {
        return false;
    }

    //
    // TODO: check region repoch
    //
    return true;
}

bool PeerMsgHandler::CheckMessage(raft_serverpb::RaftMessage* msg)
{
    auto fromEpoch = msg->region_epoch();
    auto isVoteMsg = (msg->message().msg_type() == eraftpb::MessageType::MsgRequestVote);
    auto fromStoreID = msg->from_peer().store_id();

    auto region = this->peer_->Region();
    auto target = msg->to_peer();

    if(target.id() < this->peer_->PeerId()) 
    {
        return true;
    }
    else if(target.id() > this->peer_->PeerId())
    {

        return true;
    }

    return false;
}

void PeerMsgHandler::HandleStaleMsg(Transport& trans, raft_serverpb::RaftMessage* msg, metapb::RegionEpoch* curEpoch, bool needGC)
{
    auto regionID = msg->region_id();
    auto fromPeer = msg->from_peer();
    auto toPeer = msg->to_peer();
    auto msgType = msg->message().msg_type();

    if(!needGC) 
    {
        // ignore
        return;
    }

    std::shared_ptr<raft_serverpb::RaftMessage> gcMsg = std::make_shared<raft_serverpb::RaftMessage>();
    gcMsg->set_region_id(regionID);
    gcMsg->set_allocated_from_peer(&fromPeer);
    gcMsg->set_allocated_to_peer(&toPeer);
    gcMsg->set_allocated_region_epoch(curEpoch);
    gcMsg->set_is_tombstone(true);
    trans.Send(gcMsg);
}

void PeerMsgHandler::HandleGCPeerMsg(raft_serverpb::RaftMessage* msg)
{

}

bool PeerMsgHandler::CheckSnapshot(raft_serverpb::RaftMessage& msg)  // TODO
{

}

void PeerMsgHandler::DestoryPeer()
{

}

metapb::Region* PeerMsgHandler::FindSiblingRegion()
{

}

void PeerMsgHandler::OnRaftGCLogTick()
{

}

void PeerMsgHandler::OnSplitRegionCheckTick()
{

}

void PeerMsgHandler::OnPrepareSplitRegion(metapb::RegionEpoch* regionEpoch, std::string splitKey, Callback* cb)
{

}

bool PeerMsgHandler::ValidateSplitRegion(metapb::RegionEpoch* epoch, std::string splitKey)
{

}

void PeerMsgHandler::OnApproximateRegionSize(uint64_t size)
{

}

void PeerMsgHandler::OnSchedulerHeartbeatTick()
{

}

void PeerMsgHandler::OnGCSnap()  //  TODO:
{

}

} // namespace kvserver
