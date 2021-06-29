#include <Kv/peer_msg_handler.h>

namespace kvserver
{

PeerMsgHandler::PeerMsgHandler(Peer* peer, GlobalContext* ctx)
{

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

leveldb::WriteBatch* PeerMsgHandler::ProcessRequest(eraftpb::Entry* entry, leveldb::WriteBatch* wb)
{

}

void PeerMsgHandler::ProcessAdminRequest(eraftpb::Entry* entry, raft_cmdpb::RaftCmdRequest, leveldb::WriteBatch* wb)
{

}

void PeerMsgHandler::ProcessConfChange(eraftpb::Entry* entry, eraftpb::ConfChange* cc, leveldb::WriteBatch* wb)
{

}

leveldb::WriteBatch* PeerMsgHandler::Process(eraftpb::Entry* entry, leveldb::WriteBatch* wb)
{

}

void PeerMsgHandler::HandleRaftReady()
{

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
