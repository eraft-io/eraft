#include <Kv/raft_store.h>

namespace kvserver
{
    
RaftStore::RaftStore(std::shared_ptr<Config> cfg)
{
    std::deque<Msg> storeSender;
    this->router_ = std::make_shared<Router>(storeSender);
    this->raftRouter_ = std::make_shared<RaftstoreRouter>(router_);
}

RaftStore::~RaftStore()
{

}

std::vector<Peer> RaftStore::LoadPeers()
{

}

void RaftStore::ClearStaleMeta(std::shared_ptr<leveldb::WriteBatch> kvWB, 
                               std::shared_ptr<leveldb::WriteBatch> raftWB, 
                               std::shared_ptr<raft_serverpb::RegionLocalState> originState)
{

}

bool RaftStore::Start()
{
    
}

bool RaftStore::StartWorkers(std::vector<Peer> peers)
{

}

void RaftStore::ShutDown()
{

}


} // namespace kvserver
