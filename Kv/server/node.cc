#include <Kv/node.h>

namespace kvserver
{

Node::Node(RaftStore* system, Config* cfg)
{

}

bool Node::Start(Engines* engines, Transport trans)
{

}

bool Node::CheckStore(Engines& engs, uint64_t* storeId)
{

}

bool Node::BootstrapStore(Engines& engs, uint64_t* storeId)
{

}

bool Node::StartNode(Engines* engs, Transport trans)
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
