#ifndef ERAFT_KV_RAFTSTORE_H_
#define ERAFT_KV_RAFTSTORE_H_

#include <Kv/Config.h>
#include <Kv/Engines.h>
#include <Kv/Router.h>
#include <Kv/ServerTransport.h>
#include <Kv/Transport.h>
#include <Kv/Msg.h>
#include <Kv/Peer.h>

#include <eraftio/metapb.pb.h>

namespace kvserver
{

struct StoreState
{
    uint64_t id_;

    std::vector<Msg> receiver_;
};


struct GlobalContext
{

    Config *cfg_;

    Engines *engine_;

    metapb::Store storeMeta_;

    // TODO: router
    Router *router_;

    // TODO: transport
    Transport trans_;

    // TODO: Scheduler Client
};

class RaftStore : Storage
{

public:

    RaftStore(); 

    RaftStore(Config* cfg);

    ~RaftStore();

    bool Start();

    bool StartWorkers(std::vector<Peer&> peers);

    void ShutDown();

private:

    GlobalContext* ctx_;

    StoreState* state_;

    Router* router_;

};

} // namespace kvserver


#endif