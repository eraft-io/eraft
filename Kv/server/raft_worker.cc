#include <Kv/raft_worker.h>
#include <Kv/peer_msg_handler.h>

#include <thread>

namespace kvserver
{
    
RaftWorker::RaftWorker(std::shared_ptr<GlobalContext> ctx, std::shared_ptr<Router> pm) 
{
    this->raftCh_ = pm->peerSender_;
    this->ctx_ = ctx;
    this->pr_ = pm;
}

void RaftWorker::BootThread()
{
    std::thread th(&RaftWorker::Run, this);
    th.detach();
}

void RaftWorker::Run() 
{
    std::map<uint64_t, std::shared_ptr<PeerState_> > peerStMap = std::map<uint64_t, std::shared_ptr<PeerState_> >{};
    // get msg from raft router, and handler it.
    while (this->raftCh_.size() > 0)
    {
        Msg m = *this->raftCh_.end();
        // handle m, call PeerMessageHandler
        std::shared_ptr<PeerState_> peerState = this->GetPeerState(peerStMap, m.regionId_);
        if(peerState == nullptr)
        {
            continue;
        }
        PeerMsgHandler pmHandler(peerState->peer_, this->ctx_);
        pmHandler.HandleMsg(m);
        this->raftCh_.pop_back();
    }
    // get peer state, and handle raft ready
    for(auto peerState : peerStMap) 
    {
        PeerMsgHandler pmHandler(peerState.second->peer_, this->ctx_);
        pmHandler.HandleRaftReady();
    }
}

RaftWorker::~RaftWorker()
{

}

std::shared_ptr<PeerState_> RaftWorker::GetPeerState(std::map<uint64_t, std::shared_ptr<PeerState_> > peersStateMap, uint64_t regionID)
{
    if(peersStateMap.find(regionID) == peersStateMap.end())
    {
        auto peer = this->pr_->Get(regionID);
        if(peer == nullptr)
        {
            return nullptr;
        }
        peersStateMap[regionID] = std::shared_ptr<PeerState_>(peer);
    }
    return peersStateMap[regionID];
}

} // namespace kvserver
