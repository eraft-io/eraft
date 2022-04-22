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

#include <network/raft_store.h>

namespace network
{

    RaftStore::RaftStore()
    {
        router_ = std::make_shared<Router>();
        raftRouter_ = std::make_shared<RaftStoreRouter>(router_);
    }

    RaftStore::RaftStore(std::shared_ptr<RaftConfig> cfg) {}

    RaftStore::~RaftStore() {}

    std::vector<std::shared_ptr<RaftPeer> > RaftStore::LoadPeers() {}

    std::shared_ptr<RaftPeer> RaftStore::GetLeader() {}

    void RaftStore::ClearStaleMeta(storage::WriteBatch &kvWB, storage::WriteBatch &raftWB,
                                   raft_messagepb::RegionLocalState &originState) {}

    bool RaftStore::Start(std::shared_ptr<metapb::Store> meta, std::shared_ptr<RaftConfig> cfg,
                          std::shared_ptr<StorageEngineInterface> engines,
                          std::shared_ptr<TransportInterface> trans) {}

    bool RaftStore::StartWorkers(std::vector<std::shared_ptr<RaftPeer> > peers) {}

    void RaftStore::ShutDown() {}

    bool RaftStore::Write() {}

    std::string RaftStore::Reader() {}

} // namespace network
