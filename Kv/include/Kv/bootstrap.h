#ifndef ERAFT_KV_BOOTSTRAP_H
#define ERAFT_KV_BOOTSTRAP_H

#include <leveldb/db.h>
#include <memory>

#include <Kv/engines.h>

#include <eraftio/metapb.pb.h>

namespace kvserver
{

static bool IsRangeEmpty(std::shared_ptr<leveldb::DB> engine, std::string startKey, std::string endKey);

static bool BootstrapStore(std::shared_ptr<Engines> engines, uint64_t clusterID, uint64_t storeID);

static std::pair<std::shared_ptr<metapb::Region>, bool> PrepareBootstrap(std::shared_ptr<Engines> engines, uint64_t storeID, uint64_t regionID, uint64_t peerID);

static bool PrepareBoostrapCluster(std::shared_ptr<Engines> engines, std::shared_ptr<metapb::Region> region);

static void WriteInitialApplyState(std::unique_ptr<leveldb::WriteBatch> kvWB, uint64_t regionID);

static void WriteInitialRaftState(std::unique_ptr<leveldb::WriteBatch> raftWB, uint64_t regionID);

static bool ClearPrepareBoostrap(std::shared_ptr<Engines> engines, uint64_t regionID);

static bool ClearPrepareBoostrapState(std::shared_ptr<Engines> engines);


} // namespace kvserver

#endif // ERAFT_KV_BOOTSTRAP_H
