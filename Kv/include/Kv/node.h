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

    Node(std::shared_ptr<RaftStore> system, std::shared_ptr<Config> cfg);

    bool Start(std::shared_ptr<Engines> engines, std::shared_ptr<Transport>);

    bool CheckStore(Engines& engs, uint64_t* storeId);

    bool BootstrapStore(Engines& engs, uint64_t* storeId);

    bool StartNode(std::shared_ptr<Engines> engines, std::shared_ptr<Transport> trans);

    bool StopNode(uint64_t storeID);

    void Stop();

    uint64_t GetStoreID();

    ~Node();

private:
    
    uint64_t clusterID_;

    std::shared_ptr<metapb::Store> store_;

    std::shared_ptr<Config> cfg_;

    std::shared_ptr<RaftStore> system_;
};

    
} // namespace kvserver


#endif