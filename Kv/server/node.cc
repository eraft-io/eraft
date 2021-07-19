#include <Kv/node.h>
#include <Kv/utils.h>
#include <Kv/bootstrap.h>
#include <Logger/Logger.h>

#include <eraftio/raft_serverpb.pb.h>

namespace kvserver
{

extern bool DoBootstrapStore(std::shared_ptr<Engines> engines, uint64_t clusterID, uint64_t storeID);

extern uint64_t gCounter;

Node::Node(std::shared_ptr<RaftStore> system, std::shared_ptr<Config> cfg)
{
    this->clusterID_ = 1;
    metapb::Store store;
    store.set_address(cfg->storeAddr_);
    this->store_ = std::make_shared<metapb::Store>(store);
    this->cfg_ = cfg;
    this->system_ = system;
}

Node::~Node()
{

}

bool Node::Start(std::shared_ptr<Engines> engines, std::shared_ptr<Transport> trans)
{
    uint64_t storeID;
    if(!this->CheckStore(*engines, &storeID))
    {
        Logger::GetInstance()->ERRORS("store id: " + std::to_string(storeID) + " not found");
        // return false;
    }
    if(storeID == kInvalidID)
    {
        this->BootstrapStore(*engines, &storeID);
    }
    this->store_->set_id(storeID);
    auto checkRes = this->CheckOrPrepareBoostrapCluster(engines, storeID);
    if(!checkRes.second)
    {
        // check boostrap error
        return false;
    }
    bool newCluster = (checkRes.first != nullptr);
    if(newCluster)
    {
        // try to boostrap cluster
        if(!this->BoostrapCluster(engines, checkRes.first, &newCluster))
        {
           return false;
        }
    }
    // TODO: put scheduler store
    if(!this->StartNode(engines, trans))
    {
        return false;
    }
    return true;
}

bool Node::CheckStore(Engines& engs, uint64_t* storeId)
{
    raft_serverpb::StoreIdent ident;
    if(!GetMeta(engs.kvDB_, VecToString(StoreIdentKey), &ident).ok())
    {
        // hey store ident meta key error
        *storeId = 0;
    }
    if(ident.cluster_id() != this->clusterID_)
    {
        *storeId = 0;
        //TODO: log cluster id mismatch
        return false;
    }
    if(ident.store_id() == kInvalidID)
    {
        *storeId = 0;
        return false;
    }
    *storeId = ident.store_id();
    return true;
}

uint64_t Node::AllocID()
{
    return BootHelper().GetInstance()->MockSchAllocID();
}

std::pair<std::shared_ptr<metapb::Region> , bool> Node::CheckOrPrepareBoostrapCluster(std::shared_ptr<Engines> engines, uint64_t storeID)
{
    raft_serverpb::RegionLocalState state;
    if(GetMeta(engines->kvDB_, VecToString(PrepareBootstrapKey), &state).ok())
    {
        return std::make_pair<std::shared_ptr<metapb::Region> , bool>(std::make_shared<metapb::Region>(state.region()), true);
    }
    if(!this->CheckClusterBoostrapped())
    {
        return std::make_pair<std::shared_ptr<metapb::Region> , bool>(nullptr, false);
    }
    else 
    {
        return std::make_pair<std::shared_ptr<metapb::Region> , bool>(nullptr, true);
    }
    return this->PrepareBootstrapCluster(engines, storeID);
}

bool Node::CheckClusterBoostrapped()
{
    // call sch to check cluster boostrapped

}

std::pair<std::shared_ptr<metapb::Region> , bool> Node::PrepareBootstrapCluster(std::shared_ptr<Engines> engines,  uint64_t storeID)
{
    auto regionID = BootHelper().GetInstance()->MockSchAllocID();
    // TODO: log regionID = , clusterID = , storeID =
    auto peerID = BootHelper().GetInstance()->MockSchAllocID();
    // TODO: log peerID =, regionID =
    return BootHelper().GetInstance()->PrepareBootstrap(engines, storeID, regionID, peerID);
}

bool Node::BoostrapCluster(std::shared_ptr<Engines> engines, std::shared_ptr<metapb::Region> firstRegion, bool* isNewCluster)
{
    auto regionID = firstRegion->id();
    
    // TODO: send boostrap to scheduler
    
}


bool Node::BootstrapStore(Engines& engs, uint64_t* storeId)
{
    auto storeID = this->AllocID();
    Logger::GetInstance()->INFO("boostrap store with storeID: " + std::to_string(storeID));
    if(!BootHelper::GetInstance()->DoBootstrapStore(std::make_shared<Engines>(engs), this->clusterID_, storeID))
    {
        Logger::GetInstance()->ERRORS("do bootstrap store error");
        return false;
    }
    *storeId = storeID;
    return true;
}

bool Node::StartNode(std::shared_ptr<Engines> engines, std::shared_ptr<Transport> trans)
{
    return this->system_->Start(this->store_, this->cfg_, engines, trans);
}

bool Node::StopNode(uint64_t storeID)
{
    this->system_->ShutDown();
}

void Node::Stop()
{
    this->StopNode(this->store_->id());
}

uint64_t Node::GetStoreID()
{
    return this->store_->id();
}

} // namespace kvserver
