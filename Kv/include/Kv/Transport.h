#ifndef ERAFT_KV_TRANSPORT_H_
#define ERAFT_KV_TRANSPORT_H_

#include <eraftio/raft_serverpb.pb.h>

namespace kvserver
{

class Transport
{

public:

    Transport(/* args */);

    bool Send(raft_serverpb::RaftMessage* msg);

    virtual ~Transport();

private:
    /* data */

};


} // namespace kvserver


#endif