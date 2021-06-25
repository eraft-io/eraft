#ifndef ERAFT_KV_ROUTER_H_
#define ERAFT_KV_ROUTER_H_

#include <Kv/Peer.h>
#include <Kv/Msg.h>


#include <eraftio/raft_serverpb.pb.h>
#include <eraftio/raft_cmdpb.pb.h>
#include <stdint.h>
#include <map>
#include <vector>

namespace kvserver
{

class Peer;

struct Msg;

struct PeerState
{
    uint32_t closed_;

    Peer *peer_;
};


class Router
{
public:

    Router(/* args */);

    Router(std::vector<Msg> storeSender);

    PeerState* Get(uint64_t regionID);

    void Register(Peer* peer);

    void Close(uint64_t regionID);

    bool Send(uint64_t regionID, Msg msg);

    void SendStore(Msg m);

    ~Router();

protected:

private:

    std::map<uint64_t, Peer*> peers_;

    std::vector<Msg> peerSender_;

    std::vector<Msg> storeSender_;

};

class RaftRouter
{

public:
    RaftRouter(/* args */);

    virtual ~RaftRouter();

    virtual bool Send(uint64_t regionID, Msg m) = 0;

    virtual bool SendRaftMessage(raft_serverpb::RaftMessage* msg) = 0;

    virtual bool SendRaftCommand(raft_cmdpb::RaftCmdRequest* req, Callback* cb) = 0;

};

class RaftstoreRouter : RaftRouter
{
public:

    RaftstoreRouter(Router *r);

    ~RaftstoreRouter();

    bool Send(uint64_t regionID, Msg m) override;

    bool SendRaftMessage(raft_serverpb::RaftMessage* msg) override;

    bool SendRaftCommand(raft_cmdpb::RaftCmdRequest* req, Callback* cb) override;

private:

    Router *router_;
};


} // namespace kvserver


#endif