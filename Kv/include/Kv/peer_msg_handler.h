#ifndef ERAFT_KV_PEER_MSG_HANDLER_H_
#define ERAFT_KV_PEER_MSG_HANDLER_H_

#include <Kv/peer.h>
#include <Kv/raft_store.h>
#include <Kv/engines.h>

#include <leveldb/write_batch.h>

#include <functional>
#include <memory>

namespace kvserver
{

static raft_cmdpb::RaftCmdRequest* NewAdminRequest(uint64_t regionID, metapb::Peer* peer);

static raft_cmdpb::RaftCmdRequest* NewCompactLogRequest(uint64_t regionID, metapb::Peer* peer, uint64_t compactIndex, uint64_t compactTerm);

class PeerMsgHandler : Peer
{
public:
    
    PeerMsgHandler(std::shared_ptr<Peer> peer, std::shared_ptr<GlobalContext> ctx);

    ~PeerMsgHandler();

    void Read();

    void HandleProposal(eraftpb::Entry* entry, std::function<void(Proposal*)>);

    leveldb::WriteBatch* ProcessRequest(eraftpb::Entry* entry, leveldb::WriteBatch* wb);

    void ProcessAdminRequest(eraftpb::Entry* entry, raft_cmdpb::RaftCmdRequest, leveldb::WriteBatch* wb);

    void ProcessConfChange(eraftpb::Entry* entry, eraftpb::ConfChange* cc, leveldb::WriteBatch* wb);

    leveldb::WriteBatch* Process(eraftpb::Entry* entry, leveldb::WriteBatch* wb);

    void HandleRaftReady();

    void HandleMsg(Msg m);

    bool PreProposeRaftCommand(raft_cmdpb::RaftCmdRequest* req);

    void ProposeAdminRequest(raft_cmdpb::RaftCmdRequest* msg, Callback* cb);

    void ProposeRequest(raft_cmdpb::RaftCmdRequest* msg, Callback* cb);

    void ProposeRaftCommand(raft_cmdpb::RaftCmdRequest* msg, Callback* cb);

    void OnTick();

    void StartTicker();

    void OnRaftBaseTick();

    void ScheduleCompactLog(uint64_t firstIndex, uint64_t truncatedIndex);

    bool OnRaftMsg(raft_serverpb::RaftMessage* msg);

    bool ValidateRaftMessage(raft_serverpb::RaftMessage* msg);

    bool CheckMessage(raft_serverpb::RaftMessage* msg);

    void HandleStaleMsg(Transport& trans, raft_serverpb::RaftMessage* msg, metapb::RegionEpoch* curEpoch, bool needGC);

    void HandleGCPeerMsg(raft_serverpb::RaftMessage* msg);

    bool CheckSnapshot(raft_serverpb::RaftMessage& msg);  // TODO

    void DestoryPeer();

    metapb::Region* FindSiblingRegion();

    void OnRaftGCLogTick();

    void OnSplitRegionCheckTick();

    void OnPrepareSplitRegion(metapb::RegionEpoch* regionEpoch, std::string splitKey, Callback* cb);

    bool ValidateSplitRegion(metapb::RegionEpoch* epoch, std::string splitKey);

    void OnApproximateRegionSize(uint64_t size);

    void OnSchedulerHeartbeatTick();

    void OnGCSnap();  //  TODO:



private:

    std::shared_ptr<Peer> peer_;

    std::shared_ptr<GlobalContext> ctx_;

};


} // namespace kvserver


#endif