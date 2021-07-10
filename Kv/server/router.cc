#include <Kv/router.h>

namespace kvserver
{

Router::Router(std::deque<Msg> storeSender) 
{
    this->storeSender_ = storeSender;
}
    
PeerState_* Router::Get(uint64_t regionID)
{
    if(this->peers_.find(regionID) != this->peers_.end()) {
        return this->peers_[regionID];
    }
    return nullptr;
}

void Router::Register(std::shared_ptr<Peer> peer) 
{
    PeerState_* ps = new PeerState_(peer);
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
    PeerState_* ps = this->Get(regionID);
    if(ps == nullptr) {
        return false; // TODO: log peer not fount
    }
    this->peerSender_.push_back(msg);
}

void Router::SendStore(Msg m) 
{
    this->storeSender_.push_front(m);
}

bool RaftstoreRouter::Send(uint64_t regionID, Msg m)
{
    this->router_->Send(regionID, m);
}

bool RaftstoreRouter::SendRaftMessage(raft_serverpb::RaftMessage* msg)
{
    Msg m(MsgType::MsgTypeRaftMessage, msg->region_id(), msg);
    this->router_->Send(msg->region_id(), m);
}

bool RaftstoreRouter::SendRaftCommand(raft_cmdpb::RaftCmdRequest* req, Callback* cb)
{
    MsgRaftCmd* cmd = new MsgRaftCmd(req, cb);
    Msg m(MsgType::MsgTypeRaftCmd, req->header().region_id(), cmd);
    this->router_->Send(req->header().region_id(), m);
}

RaftstoreRouter::RaftstoreRouter(std::shared_ptr<Router> r)
{
    this->router_ = r;
}

RaftstoreRouter::~RaftstoreRouter()
{
}

}// namespace kvserver
