#include <Kv/bootstrap.h>
#include <Kv/utils.h>
#include <eraftio/raft_serverpb.pb.h>
#include <eraftio/metapb.pb.h>

#include <Logger/Logger.h>

namespace kvserver
{

BootHelper* BootHelper::instance_= nullptr;
uint64_t BootHelper::gCounter_ = 0;

bool BootHelper::IsRangeEmpty(rocksdb::DB* db, std::string startKey, std::string endKey)
{
    bool hasData;
    hasData = false;
    auto it = db->NewIterator(rocksdb::ReadOptions());
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
    auto ident = new raft_serverpb::StoreIdent();
    if(!IsRangeEmpty(engines->kvDB_, "", 
        std::string(Assistant::GetInstance()->MaxKey.begin(), Assistant::GetInstance()->MaxKey.end())))
    {
        Logger::GetInstance()->ERRORS("kv db is not empty");
        return false;
    }
    if(!IsRangeEmpty(engines->raftDB_, "", 
        std::string(Assistant::GetInstance()->MaxKey.begin(), Assistant::GetInstance()->MaxKey.end())))
    {
        Logger::GetInstance()->ERRORS("raft db is not empty");
        return false;
    }
    ident->set_cluster_id(clusterID);
    ident->set_store_id(storeID);
    Assistant::GetInstance()->PutMeta(engines->kvDB_, std::string(Assistant::GetInstance()->StoreIdentKey.begin(), Assistant::GetInstance()->StoreIdentKey.end()), *ident);
    return true;
}

std::pair<std::shared_ptr<metapb::Region>, bool> BootHelper::PrepareBootstrap(std::shared_ptr<Engines> engines, uint64_t storeID, uint64_t regionID, uint64_t peerID)
{
    std::shared_ptr<metapb::Region> region = std::make_shared<metapb::Region>();
    region->set_id(regionID);
    region->set_start_key("");
    region->set_end_key("");
    metapb::RegionEpoch* epoch = new metapb::RegionEpoch();
    epoch->set_version(kInitEpochVer);
    epoch->set_conf_ver(kInitEpochConfVer);
    region->set_allocated_region_epoch(epoch);
    auto addPeer = region->add_peers();
    addPeer->set_id(peerID);
    addPeer->set_store_id(storeID);
    assert(PrepareBoostrapCluster(engines, region));
    return std::make_pair(region, true);
}

bool BootHelper::PrepareBoostrapCluster(std::shared_ptr<Engines> engines, std::shared_ptr<metapb::Region> region)
{
    raft_serverpb::RegionLocalState* state = new raft_serverpb::RegionLocalState();
    metapb::Region* region_ = new metapb::Region();
    region_->set_id(region->id());
    region_->set_start_key(region->start_key());
    region_->set_end_key(region->end_key());
    state->set_allocated_region(region_);
    rocksdb::WriteBatch kvWB;
    std::string prepareBootstrapKey(Assistant::GetInstance()->PrepareBootstrapKey.begin(), Assistant::GetInstance()->PrepareBootstrapKey.end());
    Assistant::GetInstance()->SetMeta(&kvWB, prepareBootstrapKey, *state);
    auto regionStateKey = Assistant::GetInstance()->RegionStateKey(region->id());
    std::string stateKey(regionStateKey.begin(), regionStateKey.end());
    Assistant::GetInstance()->SetMeta(&kvWB, stateKey, *state);
    WriteInitialApplyState(&kvWB, region->id());
    engines->kvDB_->Write(rocksdb::WriteOptions(), &kvWB);
    rocksdb::WriteBatch raftWB;
    WriteInitialRaftState(&raftWB, region->id());
    engines->raftDB_->Write(rocksdb::WriteOptions(), &raftWB);
    return true;
}

// write initial apply state to rocksdb batch kvWB
void BootHelper::WriteInitialApplyState(rocksdb::WriteBatch* kvWB, uint64_t regionID)
{
    raft_serverpb::RaftApplyState* applyState = new raft_serverpb::RaftApplyState();
    raft_serverpb::RaftTruncatedState* truncatedState = new raft_serverpb::RaftTruncatedState();
    applyState->set_applied_index(Assistant::GetInstance()->kRaftInitLogIndex);
    truncatedState->set_index(Assistant::GetInstance()->kRaftInitLogIndex);
    truncatedState->set_term(Assistant::GetInstance()->kRaftInitLogTerm);
    applyState->set_allocated_truncated_state(truncatedState);
    Assistant::GetInstance()->SetMeta(kvWB, std::string(Assistant::GetInstance()->ApplyStateKey(regionID).begin(), 
        Assistant::GetInstance()->ApplyStateKey(regionID).end()), *applyState);
}

// write initial raft state to raft batch, logindex = 5, logterm = 5
void BootHelper::WriteInitialRaftState(rocksdb::WriteBatch* raftWB, uint64_t regionID)
{
    raft_serverpb::RaftLocalState* raftState = new raft_serverpb::RaftLocalState();
    eraftpb::HardState hardState;
    hardState.set_term(Assistant::GetInstance()->kRaftInitLogTerm);
    hardState.set_commit(Assistant::GetInstance()->kRaftInitLogIndex);
    raftState->set_last_index(Assistant::GetInstance()->kRaftInitLogIndex);
    Assistant::GetInstance()->SetMeta(raftWB, std::string(Assistant::GetInstance()->RaftStateKey(regionID).begin(), 
        Assistant::GetInstance()->RaftStateKey(regionID).end()), *raftState);
}

bool BootHelper::ClearPrepareBoostrap(std::shared_ptr<Engines> engines, uint64_t regionID)
{
    engines->raftDB_->Delete(rocksdb::WriteOptions(), std::string(Assistant::GetInstance()->RaftStateKey(regionID).begin(), Assistant::GetInstance()->RaftStateKey(regionID).end()));
    std::shared_ptr<rocksdb::WriteBatch> wb = std::make_shared<rocksdb::WriteBatch>();
    wb->Delete(Assistant::GetInstance()->VecToString(Assistant::GetInstance()->PrepareBootstrapKey));
    wb->Delete(Assistant::GetInstance()->VecToString(Assistant::GetInstance()->RegionStateKey(regionID)));
    wb->Delete(Assistant::GetInstance()->VecToString(Assistant::GetInstance()->ApplyStateKey(regionID)));
    engines->kvDB_->Write(rocksdb::WriteOptions(),& *wb);
    return true;
}

bool BootHelper::ClearPrepareBoostrapState(std::shared_ptr<Engines> engines)
{
    engines->kvDB_->Delete(rocksdb::WriteOptions(), Assistant::GetInstance()->VecToString(Assistant::GetInstance()->PrepareBootstrapKey));
    return true;
}

} // namespace kvserver
