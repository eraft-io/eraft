#include <Kv/raft_store.h>

namespace kvserver
{
    
RaftStore::RaftStore(std::shared_ptr<Config> cfg)
{

}

RaftStore::~RaftStore()
{

}

std::vector<Peer* > RaftStore::LoadPeers()
{

}

void RaftStore::ClearStaleMeta(leveldb::WriteBatch* kvWB, leveldb::WriteBatch* raftWB, raft_serverpb::RegionLocalState* originState)
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
