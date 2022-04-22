// MIT License

// Copyright (c) 2022 eraft dev group

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <network/raft_node.h>

namespace network
{

    RaftNode::RaftNode(std::shared_ptr<RaftStore> system, std::shared_ptr<RaftConfig> cfg) {}

    bool RaftNode::Start(std::shared_ptr<DBEngines> engines, std::shared_ptr<TransportInterface> trans) {}

    bool RaftNode::CheckStore(DBEngines &engs, uint64_t *storeId) {}

    bool RaftNode::BootstrapStore(DBEngines &engs, uint64_t *storeId) {}

    uint64_t RaftNode::AllocID() {}

    std::pair<std::shared_ptr<metapb::Region>, bool>
    RaftNode::CheckOrPrepareBoostrapCluster(std::shared_ptr<DBEngines> engines, uint64_t storeId) {}

    bool RaftNode::CheckClusterBoostrapped() {}

    std::pair<std::shared_ptr<metapb::Region>, bool> RaftNode::PrepareBootstrapCluster(
        std::shared_ptr<DBEngines> engines, uint64_t storeId) {}

    bool RaftNode::BoostrapCluster(std::shared_ptr<DBEngines> engines, std::shared_ptr<metapb::Region> firstRegion, bool *isNewCluster) {}

    bool RaftNode::StartNode(std::shared_ptr<DBEngines> engines, std::shared_ptr<TransportInterface> trans)
    {
        return raftSystem_->Start(store_, raftCfg_, engines, trans);
    }

    void RaftNode::Stop() {}

    uint64_t RaftNode::GetStoreID() {}

    RaftNode::~RaftNode() {}

} // namespace network
