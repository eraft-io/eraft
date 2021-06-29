#ifndef ERAFT_KV_RAFTSTORE_H_
#define ERAFT_KV_RAFTSTORE_H_

#include <Kv/config.h>
#include <Kv/engines.h>
#include <Kv/router.h>
#include <Kv/server_transport.h>
#include <Kv/transport.h>
#include <Kv/msg.h>
#include <Kv/peer.h>
#include <Kv/storage.h>

#include <eraftio/metapb.pb.h>

#include <deque>
#include <memory>

namespace kvserver
{

class Router;

class RaftstoreRouter;

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

    Router *router_;

    Transport trans_;

    // TODO: Scheduler Client
};

class RaftStore
{

friend class RaftStorage;

public:

    RaftStore();

    RaftStore(std::shared_ptr<Config> cfg);

    ~RaftStore();

    std::vector<Peer* > LoadPeers();

    void ClearStaleMeta(leveldb::WriteBatch* kvWB, leveldb::WriteBatch* raftWB, raft_serverpb::RegionLocalState* originState);
    
    bool Start();

    bool StartWorkers(std::vector<Peer> peers);

    void ShutDown();

    bool Write(kvrpcpb::Context *ctx, std::vector<Modify>);

    StorageReader* Reader(kvrpcpb::Context *ctx);

private:

    GlobalContext* ctx_;

    StoreState* state_;


    std::shared_ptr<Router> router_;

    std::shared_ptr<RaftstoreRouter> raftRouter_;

    // scheduler client: TODO
    std::deque<uint64_t> tickDriverSender_;

    std::atomic<bool> close_;

};

} // namespace kvserver


#endif