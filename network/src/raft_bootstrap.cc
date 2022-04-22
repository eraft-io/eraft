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

#include <network/raft_bootstrap.h>
#include <network/raft_encode_assistant.h>
#include <memory>
#include <eraftio/raft_messagepb.pb.h>
#include <spdlog/spdlog.h>

namespace network
{

    BootHepler *BootHepler::instance_ = nullptr;

    uint64_t BootHepler::gCounter_ = 0;

    BootHepler::BootHepler() {}

    BootHepler::~BootHepler() {}

    bool BootHepler::IsRangeEmpty(std::shared_ptr<StorageEngineInterface> db,
                                  std::string startKey, std::string endKey)
    {
        // TODO: checkout is range empty in db
    }

    bool BootHepler::DoBootstrapStore(std::shared_ptr<DBEngines> engines, uint64_t clusterId,
                                      uint64_t storeId, std::string storeAddr)
    {
        std::shared_ptr<raft_messagepb::StoreIdent> storeIdent(new raft_messagepb::StoreIdent());
        if (!IsRangeEmpty(engines->kvDB_, "", std::string(RaftEncodeAssistant::GetInstance()->MaxKey.begin(), RaftEncodeAssistant::GetInstance()->MaxKey.end())))
        {
            SPDLOG_ERROR("kv db is not empty");
            return false;
        }
        // check raft db is empty

        storeIdent->set_cluster_id(clusterId);
    }

    uint64_t BootHepler::AllocID()
    {
        gCounter_++;
        return gCounter_;
    }

    std::pair<std::shared_ptr<metapb::Region>, bool> BootHepler::PrepareBootstrap(
        std::shared_ptr<DBEngines> engines, std::string storeAddr,
        std::map<std::string, int> peerAddrMaps) {}

    bool BootHepler::PrepareBoostrapCluster(std::shared_ptr<DBEngines> engines, std::shared_ptr<metapb::Region> region) {}

    void BootHepler::WriteInitialApplyState(storage::WriteBatch &kvWB, uint64_t regionId) {}

    void BootHepler::WriteInitialRaftState(storage::WriteBatch &raftWB, uint64_t regionId) {}

    BootHepler *BootHepler::GetInstance()
    {
        if (instance_ == nullptr)
        {
            instance_ = new BootHepler();
        }
        return instance_;
    }

} // namespace network
