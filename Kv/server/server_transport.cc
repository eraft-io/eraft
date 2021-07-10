#include <Kv/server_transport.h>

namespace kvserver
{

ServerTransport::ServerTransport(std::shared_ptr<RaftClient> raftClient, std::shared_ptr<RaftRouter> raftRouter)
{
    this->raftClient_ = raftClient;
    this->raftRouter_ = raftRouter;   
}

ServerTransport::~ServerTransport()
{

}

bool ServerTransport::Send(std::shared_ptr<raft_serverpb::RaftMessage> msg)
{
    auto storeID = msg->to_peer().store_id();
    this->SendStore(storeID, msg);
    return true;
}

void ServerTransport::SendStore(uint64_t storeID, std::shared_ptr<raft_serverpb::RaftMessage> msg)
{
    auto addr = this->raftClient_->GetAddr(storeID);
    if(addr != "")
    {
        this->WriteData(storeID, addr, msg);
        return;
    }
    if(this->resolving_.find(storeID) != this->resolving_.end())
    {
        // store address is being resolved
        return;
    }
    this->resolving_.insert(std::pair<uint64_t, void*>(storeID, 0));
    this->Resolve(storeID, msg);
}

void ServerTransport::Resolve(uint64_t storeID, std::shared_ptr<raft_serverpb::RaftMessage> msg)
{
    this->resolving_.erase(storeID);
    // this->raftClient_->InsertAddr(storeID, add);
}

void ServerTransport::WriteData(uint64_t storeID, std::string addr, std::shared_ptr<raft_serverpb::RaftMessage> msg)
{
    // TODO: send snap
    this->raftClient_->Send(storeID, addr, *msg);
}

void ServerTransport::SendSnapshotSock(std::string addr, std::shared_ptr<raft_serverpb::RaftMessage> msg)
{
    // TODO:
}

void ServerTransport::Flush()
{
    this->raftClient_->Flush();
}


} // namespace kvserver
