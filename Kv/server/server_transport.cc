#include <Kv/server_transport.h>

namespace kvserver
{

ServerTransport::ServerTransport(std::shared_ptr<RaftClient> raftClient, std::shared_ptr<RaftRouter> raftRouter)
{

}

ServerTransport::~ServerTransport()
{

}

bool ServerTransport::Send(std::shared_ptr<raft_serverpb::RaftMessage> msg)
{

}

void ServerTransport::SendStore(uint64_t storeID, std::shared_ptr<raft_serverpb::RaftMessage> msg)
{

}

void ServerTransport::Resolve(uint64_t storeID, std::shared_ptr<raft_serverpb::RaftMessage> msg)
{

}

void ServerTransport::WriteData(uint64_t storeID, std::string addr, std::shared_ptr<raft_serverpb::RaftMessage> msg)
{

}

void ServerTransport::SendSnapshotSock(std::string addr, std::shared_ptr<raft_serverpb::RaftMessage> msg)
{

}

void ServerTransport::Flush()
{

}


} // namespace kvserver
