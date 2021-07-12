#ifndef ERAFT_KV_BOOTSTRAP_H
#define ERAFT_KV_BOOTSTRAP_H

#include <rocksdb/db.h>
#include <memory>

#include <Kv/engines.h>

#include <eraftio/metapb.pb.h>

namespace kvserver
{

class BootHelper
{

protected:

    static BootHelper* instance_;

    static uint64_t gCounter_;

public:

    static const uint64_t kInitEpochVer = 1;
    static const uint64_t kInitEpochConfVer = 1;

    BootHelper() {};

    ~BootHelper() {delete instance_; };

    static bool IsRangeEmpty(rocksdb::DB* db, std::string startKey, std::string endKey);

    static bool DoBootstrapStore(std::shared_ptr<Engines> engines, uint64_t clusterID, uint64_t storeID);

    static uint64_t MockSchAllocID();

    static std::pair<std::shared_ptr<metapb::Region>, bool> PrepareBootstrap(
        std::shared_ptr<Engines> engines, uint64_t storeID, uint64_t regionID, uint64_t peerID);

    static bool PrepareBoostrapCluster(std::shared_ptr<Engines> engines, std::shared_ptr<metapb::Region> region);

    static void WriteInitialApplyState(std::shared_ptr<rocksdb::WriteBatch> kvWB, uint64_t regionID);

    static void WriteInitialRaftState(std::shared_ptr<rocksdb::WriteBatch> raftWB, uint64_t regionID);

    static bool ClearPrepareBoostrap(std::shared_ptr<Engines> engines, uint64_t regionID);

    static bool ClearPrepareBoostrapState(std::shared_ptr<Engines> engines);

    static BootHelper* GetInstance();
};




} // namespace kvserver

#endif // ERAFT_KV_BOOTSTRAP_H
