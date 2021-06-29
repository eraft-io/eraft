#ifndef ERAFT_KV_SERVER_TRANSPORT_H_
#define ERAFT_KV_SERVER_TRANSPORT_H_

#include <Kv/transport.h>

namespace kvserver
{
    
class ServerTransport : Transport
{
public:
    ServerTransport(/* args */);
    ~ServerTransport();

    bool Send(raft_serverpb::RaftMessage* msg);

private:
    

};


} // namespace kvserver


#endif