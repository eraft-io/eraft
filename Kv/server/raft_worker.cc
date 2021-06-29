#include <Kv/raft_worker.h>

namespace kvserver
{
    
RaftWorker::RaftWorker(std::shared_ptr<GlobalContext> ctx, std::shared_ptr<Router> pm) 
{
    this->raftCh_ = pm->peerSender_;
    this->ctx_ = ctx;
    this->pr_ = pm;
}

void RaftWorker::Run() 
{
    while (this->raftCh_.size() > 0)
    {
        Msg m = *this->raftCh_.end();
        // handle m, call PeerMessageHandler

        this->raftCh_.pop_back();
    }
     
}

RaftWorker::~RaftWorker()
{

}

std::shared_ptr<PeerState_> RaftWorker::GetPeerState(std::map<uint64_t, std::shared_ptr<PeerState_> > peersMap, uint64_t regionID)
{

}



} // namespace kvserver
