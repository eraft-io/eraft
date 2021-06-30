#ifndef ERAFT_KV_SERVER_TRANSPORT_H_
#define ERAFT_KV_SERVER_TRANSPORT_H_

#include <Kv/transport.h>
#include <Kv/raft_client.h>
#include <Kv/router.h>

#include <memory>
#include <string>

namespace kvserver
{

class RaftRouter;

class ServerTransport : public Transport
{
public:

    ServerTransport(std::shared_ptr<RaftClient> raftClient, std::shared_ptr<RaftRouter> raftRouter);

    ~ServerTransport();

    bool Send(std::shared_ptr<raft_serverpb::RaftMessage> msg);

    void SendStore(uint64_t storeID, std::shared_ptr<raft_serverpb::RaftMessage> msg);

    void Resolve(uint64_t storeID, std::shared_ptr<raft_serverpb::RaftMessage> msg);

    void WriteData(uint64_t storeID, std::string addr, std::shared_ptr<raft_serverpb::RaftMessage> msg);

    void SendSnapshotSock(std::string addr, std::shared_ptr<raft_serverpb::RaftMessage> msg);

    void Flush();

private:
    

};


} // namespace kvserver


#endif