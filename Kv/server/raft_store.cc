#include <Kv/raft_store.h>
#include <Kv/utils.h>
#include <Kv/store_worker.h>

#include <Logger/Logger.h>

namespace kvserver
{
    
RaftStore::RaftStore(std::shared_ptr<Config> cfg)
{
    Logger::GetInstance()->INFO("raft store init.");
    std::deque<Msg> storeSender;
    this->router_ = std::make_shared<Router>(storeSender);
    this->raftRouter_ = std::make_shared<RaftstoreRouter>(router_);
}

RaftStore::~RaftStore()
{

}

// load peers in this store. It scans the db engine, loads all regions and their peers from it
std::vector<std::shared_ptr<Peer> > RaftStore::LoadPeers()
{
    auto startKey = RegionMetaMinKey;
    auto endKey = RegionMetaMaxKey;
    auto ctx = this->ctx_;
    auto kvEngine = ctx->engine_->kvDB_;
    auto storeID = ctx->store_->id();

    uint64_t totalCount, tombStoneCount;
    std::vector<std::shared_ptr<Peer> > regionPeers;

    std::shared_ptr<rocksdb::WriteBatch> kvWB = std::make_shared<rocksdb::WriteBatch>();
    std::shared_ptr<rocksdb::WriteBatch> raftWB = std::make_shared<rocksdb::WriteBatch>();

    auto iter = kvEngine->NewIterator(rocksdb::ReadOptions());
    for(iter->Seek(VecToString(startKey)); iter->Valid(); iter->Next())
    {
        if(iter->key().ToString().compare(VecToString(endKey)) >= 0)
        {
            break;
        }
        uint64_t regionID; 
        uint8_t suffix;
        DecodeRegionMetaKey(StringToVec(iter->key().ToString()), &regionID, &suffix);
        if(suffix != kRegionStateSuffix[0]) // filter other key
        {
            continue;
        }
        auto val = iter->value().ToString();
        totalCount++;
        std::shared_ptr<raft_serverpb::RegionLocalState> localState = std::make_shared<raft_serverpb::RegionLocalState>();
        localState->ParseFromString(val);
        auto region = localState->region();
        if(localState->state() == raft_serverpb::PeerState::Tombstone)
        {
            tombStoneCount++;
            this->ClearStaleMeta(kvWB, raftWB, localState);
            continue;
        }

        std::shared_ptr<Peer> peer = std::make_shared<Peer>(storeID, ctx->cfg_, ctx->engine_, std::make_shared<metapb::Region>(region));

        // TODO: store region range to storeMeta
        // ctx->storeMeta_->regionRanges_(region);
        ctx->storeMeta_->regions_[regionID] = &region;

        regionPeers.push_back(peer);
    }

    ctx_->engine_->kvDB_->Write(rocksdb::WriteOptions(),& *kvWB);
    ctx_->engine_->raftDB_->Write(rocksdb::WriteOptions(),& *raftWB);

    return regionPeers;
}

void RaftStore::ClearStaleMeta(std::shared_ptr<rocksdb::WriteBatch> kvWB, 
                               std::shared_ptr<rocksdb::WriteBatch> raftWB, 
                               std::shared_ptr<raft_serverpb::RegionLocalState> originState)
{
    auto region = originState->region();
    auto stateRes = GetRaftLocalState(this->ctx_->engine_->raftDB_, region.id());
    auto status = stateRes.second;
    auto raftState = stateRes.first;
    if(!status.ok())
    {
        // it has be cleaned up
        return;
    }
    if(!DoClearMeta(this->ctx_->engine_, kvWB, raftWB, region.id(), raftState->last_index()))
    {
        return;
    }
    if(!SetMeta(kvWB.get(), VecToString(RegionStateKey(region.id())), *originState))
    {
        // TODO log error
        return;
    }
}

bool RaftStore::Start(std::shared_ptr<metapb::Store> meta,
                      std::shared_ptr<Config> cfg,
                      std::shared_ptr<Engines> engines,
                      std::shared_ptr<Transport> trans)
{
    assert(cfg->Validate());

    std::shared_ptr<StoreMeta> storeMeta = std::make_shared<StoreMeta>();

    this->ctx_ = std::make_shared<GlobalContext>(cfg, engines, meta, storeMeta, this->router_, trans);

    // register peer
    auto regionPeers = this->LoadPeers();

    for(auto peer : regionPeers)
    {
        this->router_->Register(peer);
    }

    this->StartWorkers(regionPeers);

    return true;
}

bool RaftStore::StartWorkers(std::vector<std::shared_ptr<Peer> > peers)
{
    auto ctx = this->ctx_;
    auto router = this->router_;
    auto state = this->state_;
    auto rw = RaftWorker(ctx, router);
    rw.BootThread();
    auto sw = StoreWorker(ctx, state);
    sw.BootThread();
    Msg m(MsgType::MsgTypeStoreStart,& *ctx->store_);
    router->SendStore(m);
    for(uint64_t i = 0; i < peers.size(); i++)
    {
        auto regionID = peers[i]->regionId_;
        Msg m(MsgType::MsgTypeStart, & *ctx->store_);
        router->Send(regionID, m);
    }
    // run ticker
}

void RaftStore::ShutDown()
{
    
}


} // namespace kvserver
