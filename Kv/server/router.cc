#include <Kv/router.h>
#include <Logger/Logger.h>

#include <RaftCore/Util.h>

namespace kvserver
{

Router::Router() 
{
}

std::shared_ptr<PeerState_> Router::Get(uint64_t regionID)
{
    if(this->peers_.find(regionID) != this->peers_.end()) {
        return this->peers_[regionID];
    }
    return nullptr;
}

void Router::Register(std::shared_ptr<Peer> peer) 
{
    Logger::GetInstance()->DEBUG_NEW("register peer regionId -> " + std::to_string(peer->regionId_) + " peer id -> " + 
    std::to_string(peer->PeerId()) +  " peer addr -> " + peer->meta_->addr() + " to router", __FILE__, __LINE__, "Router::Register");

    std::shared_ptr<PeerState_> ps = std::make_shared<PeerState_>(peer);
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
    // std::shared_ptr<PeerState_> ps = this->Get(regionID);
    // if(ps == nullptr) {
    //     return false; // TODO: log peer not found
    // }
    Logger::GetInstance()->DEBUG_NEW("push raft msg to peer sender, type" + msg.MsgToString() , __FILE__, __LINE__, "Router::Register");
    QueueContext::GetInstance()->peerSender_.Push(msg);
    return true;
}

void Router::SendStore(Msg m) 
{
    QueueContext::GetInstance()->storeSender_.Push(m);
}

bool RaftstoreRouter::Send(uint64_t regionID, const Msg m)
{
    this->router_->Send(regionID, m);
}

bool RaftstoreRouter::SendRaftMessage(const raft_serverpb::RaftMessage* msg)
{
    Logger::GetInstance()->DEBUG_NEW("send raft message type " + eraft::MsgTypeToString(msg->message().msg_type()) , __FILE__, __LINE__, "RaftstoreRouter::SendRaftMessage");
    Msg m = Msg(MsgType::MsgTypeRaftMessage, msg->region_id(), const_cast<raft_serverpb::RaftMessage*>(msg));
    return this->router_->Send(msg->region_id(), m);
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
