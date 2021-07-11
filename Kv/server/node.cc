#include <Kv/node.h>
#include <Kv/utils.h>
#include <Kv/bootstrap.h>

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
        return false;
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
        return false;
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
    return MockSchAllocID();
}

std::pair<std::shared_ptr<metapb::Region> , bool> Node::CheckOrPrepareBoostrapCluster(std::shared_ptr<Engines> engines, uint64_t storeID)
{
    raft_serverpb::RegionLocalState state;
    GetMeta(engines->kvDB_, VecToString(PrepareBootstrapKey), &state);
}

bool Node::CheckClusterBoostrapped()
{

}

std::pair<std::shared_ptr<metapb::Region> , bool> Node::PrepareBootstrapCluster(std::shared_ptr<Engines> engines,  uint64_t storeID)
{

}

bool Node::BoostrapCluster(std::shared_ptr<Engines> engines, std::shared_ptr<metapb::Region> firstRegion, bool* isNewCluster)
{

}


bool Node::BootstrapStore(Engines& engs, uint64_t* storeId)
{
    auto storeID = this->AllocID();
    BootHelper::GetInstance()->DoBootstrapStore(std::make_shared<Engines>(engs), this->clusterID_, storeID);
}

bool Node::StartNode(std::shared_ptr<Engines> engines, std::shared_ptr<Transport>)
{
    
}

bool Node::StopNode(uint64_t storeID)
{

}

void Node::Stop()
{

}

uint64_t Node::GetStoreID()
{
    
}

} // namespace kvserver
