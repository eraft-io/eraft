#ifndef ERAFT_KV_ROUTER_H_
#define ERAFT_KV_ROUTER_H_

#include <Kv/peer.h>
#include <Kv/msg.h>
#include <Kv/raft_worker.h>
#include <Kv/server_transport.h>

#include <eraftio/raft_serverpb.pb.h>
#include <eraftio/raft_cmdpb.pb.h>
#include <stdint.h>
#include <map>
#include <vector>
#include <deque>
#include <atomic>
#include <memory>

#include <Kv/concurrency_queue.h>

namespace kvserver
{

class Peer;

struct Msg;

struct PeerState_
{
    PeerState_(std::shared_ptr<Peer> peer) {
        this->closed_ = 0;
        this->peer_ = peer;
    }

    std::atomic<uint32_t> closed_;

    std::shared_ptr<Peer> peer_;
};


class Router
{

friend class RaftWorker;
friend class ServerTransport;

public:

    Router();

    std::shared_ptr<PeerState_> Get(uint64_t regionID);

    void Register(std::shared_ptr<Peer> peer);

    void Close(uint64_t regionID);

    bool Send(uint64_t regionID, Msg msg);

    void SendStore(Msg m);

    ~Router() {}

protected:

    std::map<uint64_t, std::shared_ptr<PeerState_> > peers_;

private:


};

class RaftRouter
{

public:

    virtual bool Send(uint64_t regionID, Msg m) = 0;

    virtual bool SendRaftMessage(const raft_serverpb::RaftMessage* msg) = 0;

    virtual bool SendRaftCommand(raft_cmdpb::RaftCmdRequest* req, Callback* cb) = 0;

};

class RaftstoreRouter : public RaftRouter
{
public:

    RaftstoreRouter(std::shared_ptr<Router> r);

    ~RaftstoreRouter();

    bool Send(uint64_t regionID, Msg m) override;

    bool SendRaftMessage(const raft_serverpb::RaftMessage* msg) override;

    bool SendRaftCommand(raft_cmdpb::RaftCmdRequest* req, Callback* cb) override;

private:

    std::shared_ptr<Router> router_;
};


} // namespace kvserver


#endif