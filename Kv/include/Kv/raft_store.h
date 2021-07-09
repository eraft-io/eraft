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

class RaftRouter;

struct StoreState
{
    uint64_t id_;

    std::vector<Msg> receiver_;
};


struct GlobalContext
{

    std::shared_ptr<Config> cfg_;

    std::shared_ptr<Engines> engine_;

    metapb::Store storeMeta_;

    std::shared_ptr<Router> router_;

    std::shared_ptr<Transport> trans_;

    // TODO: Scheduler Client
};

class RaftStore
{

friend class RaftStorage;

public:

    RaftStore();

    RaftStore(std::shared_ptr<Config> cfg);

    ~RaftStore();

    std::vector<Peer> LoadPeers();

    void ClearStaleMeta(std::shared_ptr<rocksdb::WriteBatch> kvWB, 
                        std::shared_ptr<rocksdb::WriteBatch> raftWB, 
                        std::shared_ptr<raft_serverpb::RegionLocalState> originState);
    
    bool Start();

    bool StartWorkers(std::vector<Peer> peers);

    void ShutDown();

    bool Write(kvrpcpb::Context *ctx, std::vector<Modify>);

    StorageReader* Reader(kvrpcpb::Context *ctx);

private:

    std::shared_ptr<GlobalContext> ctx_;

    std::shared_ptr<StoreState> state_;


    std::shared_ptr<Router> router_;

    std::shared_ptr<RaftRouter> raftRouter_;

    // scheduler client: TODO
    std::deque<uint64_t> tickDriverSender_;

    std::atomic<bool> close_;

};

} // namespace kvserver


#endif