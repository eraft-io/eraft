#ifndef ERAFT_KV_TRANSPORT_H_
#define ERAFT_KV_TRANSPORT_H_

#include <memory>

#include <eraftio/raft_serverpb.pb.h>
namespace kvserver
{

class Transport
{

public:

    virtual bool Send(std::shared_ptr<raft_serverpb::RaftMessage> msgg) = 0;

    // virtual ~Transport();

private:
    /* data */

};


} // namespace kvserver


#endif