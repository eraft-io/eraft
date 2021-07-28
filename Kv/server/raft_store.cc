#include <chrono>

#include <Kv/raft_store.h>
#include <Kv/utils.h>
#include <Kv/store_worker.h>
#include <Kv/ticker.h>
#include <Logger/Logger.h>
#include <RaftCore/Util.h>

namespace kvserver
{
    
RaftStore::RaftStore(std::shared_ptr<Config> cfg)
{
    this->router_ = std::make_shared<Router>();
    this->raftRouter_ = std::make_shared<RaftstoreRouter>(router_);
    Logger::GetInstance()->DEBUG_NEW("raft store init success", __FILE__, __LINE__, "RaftStore::RaftStore");
}

RaftStore::~RaftStore()
{

}

// load peers in this store. It scans the db engine, loads all regions and their peers from it
std::vector<std::shared_ptr<Peer> > RaftStore::LoadPeers()
{
    auto startKey = Assistant::GetInstance()->RegionMetaMinKey;
    auto endKey = Assistant::GetInstance()->RegionMetaMaxKey;
    auto ctx = this->ctx_;
    auto kvEngine = ctx->engine_->kvDB_;
    auto storeID = ctx->store_->id();
    Logger::GetInstance()->DEBUG_NEW("raft store store id: "  + std::to_string(storeID) + " load peers", __FILE__, __LINE__, "RaftStore::LoadPeers");
    uint64_t totalCount, tombStoneCount;
    std::vector<std::shared_ptr<Peer> > regionPeers;

    rocksdb::WriteBatch kvWB;
    rocksdb::WriteBatch raftWB;

    auto iter = kvEngine->NewIterator(rocksdb::ReadOptions());
    for(iter->Seek(Assistant::GetInstance()->VecToString(startKey)); iter->Valid(); iter->Next())
    {
        if(iter->key().ToString().compare(Assistant::GetInstance()->VecToString(endKey)) >= 0)
        {
            break;
        }
        uint64_t regionID; 
        uint8_t suffix;
        Assistant::GetInstance()->DecodeRegionMetaKey(Assistant::GetInstance()->StringToVec(iter->key().ToString()), 
            &regionID, &suffix);
        if(suffix != Assistant::GetInstance()->kRegionStateSuffix[0]) // filter other key
        {
            continue;
        }        
        auto val = iter->value().ToString();
        totalCount++;
        raft_serverpb::RegionLocalState* localState = new raft_serverpb::RegionLocalState();
        localState->ParseFromString(val);

        // debug msg
        std::string debugVal;
        google::protobuf::TextFormat::PrintToString(*localState, &debugVal);
        Logger::GetInstance()->DEBUG_NEW("local state: " + debugVal, __FILE__, __LINE__, "RaftStore::LoadPeers");

        auto region = localState->region();

        if(localState->state() == raft_serverpb::PeerState::Tombstone)
        {
            tombStoneCount++;
            this->ClearStaleMeta(&kvWB, &raftWB, localState);
            continue;
        }

        std::shared_ptr<Peer> peer = std::make_shared<Peer>(storeID, ctx->cfg_, ctx->engine_, std::make_shared<metapb::Region>(region));

        ctx->storeMeta_->regions_[regionID] = &region;
        regionPeers.push_back(peer);
    }

    ctx_->engine_->kvDB_->Write(rocksdb::WriteOptions(), &kvWB);
    ctx_->engine_->raftDB_->Write(rocksdb::WriteOptions(), &raftWB);

    return regionPeers;
}

void RaftStore::ClearStaleMeta(rocksdb::WriteBatch* kvWB, 
                               rocksdb::WriteBatch* raftWB, 
                                raft_serverpb::RegionLocalState* originState)
{
    auto region = originState->region();
    auto stateRes = Assistant::GetInstance()->GetRaftLocalState(this->ctx_->engine_->raftDB_, region.id());
    auto status = stateRes.second;
    auto raftState = stateRes.first;
    if(!status.ok())
    {
        return;
    }
    if(!Assistant::GetInstance()->DoClearMeta(this->ctx_->engine_, kvWB, raftWB, region.id(), raftState->last_index()))
    {
        return;
    }
    if(!Assistant::GetInstance()->SetMeta(kvWB, Assistant::GetInstance()->RegionStateKey(region.id()), *originState))
    {
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
    Msg m(MsgType::MsgTypeStoreStart, ctx->store_.get());
    router->SendStore(m);
    for(uint64_t i = 0; i < peers.size(); i++)
    {
        auto regionID = peers[i]->regionId_;
        Msg m(MsgType::MsgTypeStart, ctx->store_.get());
        router->Send(regionID, m);
    }

    // ticker start
    std::chrono::duration<int, std::milli> timer_duration1(1000); // 1s
    Ticker::GetInstance(std::function<void()>(Ticker::Run), router ,timer_duration1)->Start();

    return true;
}

void RaftStore::ShutDown()
{

}

} // namespace kvserver
