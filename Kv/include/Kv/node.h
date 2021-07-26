#ifndef ERAFT_KV_NODE_H_
#define ERAFT_KV_NODE_H_

#include <stdint.h>
#include <eraftio/metapb.pb.h>

#include <Kv/config.h>
#include <Kv/raft_store.h>
#include <Kv/engines.h>
#include <Kv/transport.h>

#include <memory>

namespace kvserver
{

class Node
{

public:

    const uint64_t kMaxCheckClusterBoostrappedRetryCount = 60;

    const uint64_t kCheckClusterBoostrapRetrySeconds = 3;

    Node(std::shared_ptr<RaftStore> system, std::shared_ptr<Config> cfg);

    bool Start(std::shared_ptr<Engines> engines, std::shared_ptr<Transport>);

    bool CheckStore(Engines& engs, uint64_t* storeId);

    bool BootstrapStore(Engines& engs, uint64_t* storeId);

    uint64_t AllocID();

    std::pair<std::shared_ptr<metapb::Region> , bool> CheckOrPrepareBoostrapCluster(std::shared_ptr<Engines> engines, uint64_t storeID);

    bool CheckClusterBoostrapped();

    std::pair<std::shared_ptr<metapb::Region> , bool> PrepareBootstrapCluster(std::shared_ptr<Engines> engines,  uint64_t storeID);

    bool BoostrapCluster(std::shared_ptr<Engines> engines, std::shared_ptr<metapb::Region> firstRegion, bool* isNewCluster);

    
    bool StartNode(std::shared_ptr<Engines> engines, std::shared_ptr<Transport> trans);

    bool StopNode(uint64_t storeID);

    void Stop();

    uint64_t GetStoreID();

    ~Node();

    std::shared_ptr<Engines> engs_;

private:
    
    uint64_t clusterID_;

    std::shared_ptr<metapb::Store> store_;

    std::shared_ptr<Config> cfg_;

    std::shared_ptr<RaftStore> system_;
};

    
} // namespace kvserver


#endif