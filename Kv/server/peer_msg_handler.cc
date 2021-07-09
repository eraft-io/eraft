#include <Kv/peer_msg_handler.h>

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

void PeerMsgHandler::HandleProposal(eraftpb::Entry* entry, std::function<void(Proposal*)>)
{

}

rocksdb::WriteBatch* PeerMsgHandler::ProcessRequest(eraftpb::Entry* entry, rocksdb::WriteBatch* wb)
{

}

void PeerMsgHandler::ProcessAdminRequest(eraftpb::Entry* entry, raft_cmdpb::RaftCmdRequest, rocksdb::WriteBatch* wb)
{

}

void PeerMsgHandler::ProcessConfChange(eraftpb::Entry* entry, eraftpb::ConfChange* cc, rocksdb::WriteBatch* wb)
{

}

rocksdb::WriteBatch* PeerMsgHandler::Process(eraftpb::Entry* entry, rocksdb::WriteBatch* wb)
{

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
            
        }
    }
}

void PeerMsgHandler::HandleMsg(Msg m)
{
    
}

bool PeerMsgHandler::PreProposeRaftCommand(raft_cmdpb::RaftCmdRequest* req)
{

}

void PeerMsgHandler::ProposeAdminRequest(raft_cmdpb::RaftCmdRequest* msg, Callback* cb)
{

}

void PeerMsgHandler::ProposeRequest(raft_cmdpb::RaftCmdRequest* msg, Callback* cb)
{

}

void PeerMsgHandler::ProposeRaftCommand(raft_cmdpb::RaftCmdRequest* msg, Callback* cb)
{

}

void PeerMsgHandler::OnTick()
{

}

void PeerMsgHandler::StartTicker()
{

}

void PeerMsgHandler::OnRaftBaseTick()
{

}

void PeerMsgHandler::ScheduleCompactLog(uint64_t firstIndex, uint64_t truncatedIndex)
{

}

bool PeerMsgHandler::OnRaftMsg(raft_serverpb::RaftMessage* msg)
{

}

bool PeerMsgHandler::ValidateRaftMessage(raft_serverpb::RaftMessage* msg)
{

}

bool PeerMsgHandler::CheckMessage(raft_serverpb::RaftMessage* msg)
{

}

void PeerMsgHandler::HandleStaleMsg(Transport& trans, raft_serverpb::RaftMessage* msg, metapb::RegionEpoch* curEpoch, bool needGC)
{

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
