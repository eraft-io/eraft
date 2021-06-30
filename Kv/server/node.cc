#include <Kv/node.h>

namespace kvserver
{

Node::Node(std::shared_ptr<RaftStore> system, std::shared_ptr<Config> cfg)
{

}

bool Node::Start(std::shared_ptr<Engines> engines, std::shared_ptr<Transport> trans)
{

    return true;
}

bool Node::CheckStore(Engines& engs, uint64_t* storeId)
{

}

bool Node::BootstrapStore(Engines& engs, uint64_t* storeId)
{

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
