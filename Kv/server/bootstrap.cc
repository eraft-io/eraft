#include <Kv/bootstrap.h>
#include <Kv/utils.h>
#include <eraftio/raft_serverpb.pb.h>
#include <eraftio/metapb.pb.h>

#include <Logger/Logger.h>

namespace kvserver
{

bool BootHelper::IsRangeEmpty(rocksdb::DB* db, std::string startKey, std::string endKey)
{
    bool hasData;
    rocksdb::Iterator* it = db->NewIterator(rocksdb::ReadOptions());
    it->Seek(startKey);
    if(it->Valid())
    {
        if(it->key().ToString().compare(endKey) < 0) 
        {
            hasData = true;
        }
    }
    return !hasData;
}

BootHelper* BootHelper::instance_= nullptr;
uint64_t BootHelper::gCounter_ = 0;

uint64_t BootHelper::MockSchAllocID()
{
    gCounter_++;
    return gCounter_;
}

BootHelper* BootHelper::GetInstance()
{
    if(instance_ == nullptr)
    {
        instance_ = new BootHelper();
        return instance_;
    }
}

bool BootHelper::DoBootstrapStore(std::shared_ptr<Engines> engines, uint64_t clusterID, uint64_t storeID)
{
    auto ident = raft_serverpb::StoreIdent();
    if(!IsRangeEmpty(engines->kvDB_, std::string(MinKey.begin(), MinKey.end()), std::string(MaxKey.begin(), MaxKey.end())))
    {
        Logger::GetInstance()->ERRORS("kv db is not empty");
        return false;
    }
    if(!IsRangeEmpty(engines->raftDB_, std::string(MinKey.begin(), MinKey.end()), std::string(MaxKey.begin(), MaxKey.end())))
    {
        Logger::GetInstance()->ERRORS("raft db is not empty");
        return false;
    }
    ident.set_cluster_id(clusterID);
    ident.set_store_id(storeID);
    PutMeta(engines->kvDB_, std::string(StoreIdentKey.begin(), StoreIdentKey.end()), ident);
    return true;
}

std::pair<std::shared_ptr<metapb::Region>, bool> BootHelper::PrepareBootstrap(std::shared_ptr<Engines> engines, uint64_t storeID, uint64_t regionID, uint64_t peerID)
{
    std::shared_ptr<metapb::Region> region = std::make_shared<metapb::Region>();
    region->set_id(regionID);
    region->set_start_key("");
    region->set_end_key("");
    metapb::RegionEpoch epoch;
    epoch.set_version(kInitEpochVer);
    epoch.set_conf_ver(kInitEpochConfVer);
    region->set_allocated_region_epoch(&epoch);
    auto addPeer = region->add_peers();
    addPeer->set_id(peerID);
    addPeer->set_store_id(storeID);
    assert(PrepareBoostrapCluster(engines, region));
    return std::make_pair(region, true);
}

bool BootHelper::PrepareBoostrapCluster(std::shared_ptr<Engines> engines, std::shared_ptr<metapb::Region> region)
{
    raft_serverpb::RegionLocalState state;
    state.set_allocated_region(& *region);
    std::shared_ptr<rocksdb::WriteBatch> kvWB = std::make_shared<rocksdb::WriteBatch>();
    SetMeta(kvWB, std::string(PrepareBootstrapKey.begin(), PrepareBootstrapKey.end()), state);
    auto stateKey = std::string(RegionStateKey(region->id()).begin(), RegionStateKey(region->id()).end());
    SetMeta(kvWB, "test", state);
    // WriteInitialApplyState(kvWB, region->id());
    // engines->kvDB_->Write(rocksdb::WriteOptions(),& *kvWB);
    // std::shared_ptr<rocksdb::WriteBatch> raftWB = std::make_shared<rocksdb::WriteBatch>();
    // WriteInitialRaftState(raftWB, region->id());
    // engines->raftDB_->Write(rocksdb::WriteOptions(),& *raftWB);
    return true;
}

void BootHelper::WriteInitialApplyState(std::shared_ptr<rocksdb::WriteBatch> kvWB, uint64_t regionID)
{
    raft_serverpb::RaftApplyState applyState;
    raft_serverpb::RaftTruncatedState truncatedState;
    applyState.set_applied_index(kRaftInitLogIndex);
    truncatedState.set_index(kRaftInitLogIndex);
    truncatedState.set_term(kRaftInitLogTerm);
    SetMeta(kvWB, std::string(ApplyStateKey(regionID).begin(), ApplyStateKey(regionID).end()), applyState);
}

void BootHelper::WriteInitialRaftState(std::shared_ptr<rocksdb::WriteBatch> raftWB, uint64_t regionID)
{
    raft_serverpb::RaftLocalState raftState;
    eraftpb::HardState hardState;
    hardState.set_term(kRaftInitLogTerm);
    hardState.set_commit(kRaftInitLogIndex);
    raftState.set_last_index(kRaftInitLogIndex);
    SetMeta(raftWB, std::string(RaftStateKey(regionID).begin(), RaftStateKey(regionID).end()), raftState);
}

bool BootHelper::ClearPrepareBoostrap(std::shared_ptr<Engines> engines, uint64_t regionID)
{
    engines->raftDB_->Delete(rocksdb::WriteOptions(), std::string(RaftStateKey(regionID).begin(), RaftStateKey(regionID).end()));
    std::shared_ptr<rocksdb::WriteBatch> wb = std::make_shared<rocksdb::WriteBatch>();
    wb->Delete(VecToString(PrepareBootstrapKey));
    wb->Delete(VecToString(RegionStateKey(regionID)));
    wb->Delete(VecToString(ApplyStateKey(regionID)));
    engines->kvDB_->Write(rocksdb::WriteOptions(),& *wb);
    return true;
}

bool BootHelper::ClearPrepareBoostrapState(std::shared_ptr<Engines> engines)
{
    engines->kvDB_->Delete(rocksdb::WriteOptions(), VecToString(PrepareBootstrapKey));
    return true;
}

} // namespace kvserver
