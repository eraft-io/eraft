#include <Kv/bootstrap.h>

namespace kvserver
{

static bool IsRangeEmpty(std::shared_ptr<leveldb::DB> engine, std::string startKey, std::string endKey)
{
    return true;
}

static bool BootstrapStore(std::shared_ptr<Engines> engines, uint64_t clusterID, uint64_t storeID)
{
    return true;
}

static std::pair<std::shared_ptr<metapb::Region>, bool> PrepareBootstrap(std::shared_ptr<Engines> engines, uint64_t storeID, uint64_t regionID, uint64_t peerID)
{
    return std::make_pair(std::make_shared<metapb::Region>(), true);
}

static bool PrepareBoostrapCluster(std::shared_ptr<Engines> engines, std::shared_ptr<metapb::Region> region)
{
    return true;
}

static void WriteInitialApplyState(std::unique_ptr<leveldb::WriteBatch> kvWB, uint64_t regionID)
{

}

static void WriteInitialRaftState(std::unique_ptr<leveldb::WriteBatch> raftWB, uint64_t regionID)
{

}

static bool ClearPrepareBoostrap(std::shared_ptr<Engines> engines, uint64_t regionID)
{
    return true;
}

static bool ClearPrepareBoostrapState(std::shared_ptr<Engines> engines)
{
    return true;
}

} // namespace kvserver
