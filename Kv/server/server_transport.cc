#include <Kv/server_transport.h>
#include <Kv/utils.h>

#include <Logger/Logger.h>
#include <google/protobuf/text_format.h>

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
    auto addr = msg->to_peer().addr();
    this->SendStore(storeID, msg, addr);
    return true;
}

void ServerTransport::SendStore(uint64_t storeID, std::shared_ptr<raft_serverpb::RaftMessage> msg, std::string addr)
{
    Logger::GetInstance()->DEBUG_NEW("send to store id " + std::to_string(storeID) + " store addr " + addr, __FILE__, __LINE__, "ServerTransport::SendStore");
    this->WriteData(storeID, addr, msg);
}

void ServerTransport::Resolve(uint64_t storeID, std::shared_ptr<raft_serverpb::RaftMessage> msg)
{
    this->resolving_.erase(storeID);
}

void ServerTransport::WriteData(uint64_t storeID, std::string addr, std::shared_ptr<raft_serverpb::RaftMessage> msg)
{
    this->raftClient_->Send(storeID, addr, *msg);
}

void ServerTransport::SendSnapshotSock(std::string addr, std::shared_ptr<raft_serverpb::RaftMessage> msg)
{

}

void ServerTransport::Flush()
{
    this->raftClient_->Flush();
}

} // namespace kvserver
