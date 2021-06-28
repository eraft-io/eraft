#include <Kv/Router.h>

namespace kvserver
{

Router::Router(std::deque<Msg> storeSender) 
{
    this->storeSender_ = storeSender;
}
    
PeerState* Router::Get(uint64_t regionID)
{
    if(this->peers_.find(regionID) != this->peers_.end()) {
        return this->peers_[regionID];
    }
    return nullptr;
}

void Router::Register(Peer* peer) 
{
    PeerState* ps = new PeerState(peer);
    this->peers_[peer->regionId_] = ps;
}

void Router::Close(uint64_t regionID) 
{
    // this->peers_[regionID]->closed_.store(1, std::memory_order_relaxed);
    this->peers_.erase(regionID);
}

bool Router::Send(uint64_t regionID, Msg msg) 
{
    msg.regionId_ = regionID;
}

void Router::SendStore(Msg m) 
{
    this->storeSender_.push(m);
}

bool RaftstoreRouter::Send(uint64_t regionID, Msg m)
{
    this->router_.Send(regionID, m);
}

bool RaftstoreRouter::SendRaftMessage(raft_serverpb::RaftMessage* msg)
{
    this->router_.Send(msg.region_id());
}

bool RaftstoreRouter::SendRaftCommand(raft_cmdpb::RaftCmdRequest* req, Callback* cb)
{
    
} // namespace kvserver
