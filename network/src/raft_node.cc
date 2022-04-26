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

#include <eraftio/metapb.pb.h>
#include <network/raft_node.h>

namespace network {

RaftNode::RaftNode(std::shared_ptr<RaftStore> system,
                   std::shared_ptr<RaftConfig> cfg)
    : clusterID_(1), raftCfg_(cfg), raftSystem_(system) {
  metapb::Region store;
  store.set_address(raftCfg_->storeAddr_);
  store_ = std::make_shared<metapb::Store>(store);
}

bool RaftNode::Start(std::shared_ptr<DBEngines> engines,
                     std::shared_ptr<TransportInterface> trans) {
  // 1.check store
  uint64_t storeID;
  if (!ChechStore(engines, &storeID)) {
    SPDLOG_ERROR("store id " + std::to_string(storeID) + " not found");
  }
  if (storeID == RaftEncodeAssistant::GetInstance()->kInvalidID) {
    BootstrapStore(*engines, &storeID);
  }
  // 2.bootstap store
  store_->set_id(storeID);

  auto checkPrepareRes = CheckOrPrepareBoostrapCluster(engines, storeID);
  if (!checkPrepareRes.second) {
    SPDLOG_ERROR("check or prepare boostrap cluster " +
                 std::to_string(storeID) + " not found");
    return false;
  }
  // 3.start node, no jump!!  system_->Start(this->store_, this->cfg_, engines,
  // trans);
  bool isNewCluster = (checkPrepareRes.first != nullptr);
  if (isNewCluster) {
    if (!BoostrapCluster(engines, checkPrepareRes.first, &isNewCluster)) {
      return false;
    }
  }
  if (StartNode(engines, trans)) {
    return false;
  }
  return true;
}

bool RaftNode::CheckStore(DBEngines &engs, uint64_t *storeId) { return true; }

bool RaftNode::BootstrapStore(DBEngines &engs, uint64_t *storeId) {
  auto storeID = raftCfg_->peerAddrMaps_[raftCfg_->storeAddr_];
  SPDLOG_INFO("boostrap store with storeID: " + std::to_string(storeId));
  if (!BootHelper::GetInstance()->DoBootstrapStore(
          std::make_shared<Engines>(engs), this->clusterID_, storeID,
          this->store_->address())) {
    SPDLOG_ERROR("do bootstrap store error!");
    return false;
  }
  *storeId = storeID;
  return true;
}

uint64_t RaftNode::AllocID() { return BootHepler::GetInstance()->AllocID(); }

std::pair<std::shared_ptr<metapb::Region>, bool>
RaftNode::CheckOrPrepareBoostrapCluster(std::shared_ptr<DBEngines> engines,
                                        uint64_t storeId) {
  PrepareBootstrapCluster(engines, storeId);
}

bool RaftNode::CheckClusterBoostrapped() { return true; }

std::pair<std::shared_ptr<metapb::Region>, bool>
RaftNode::PrepareBootstrapCluster(std::shared_ptr<DBEngines> engines,
                                  uint64_t storeId) {
  return BootHepler::GetInstance()->PrepareBootstrap(engines, store_->address(),
                                                     raftCfg_->peerAddrMaps_);
}

bool RaftNode::BoostrapCluster(std::shared_ptr<DBEngines> engines,
                               std::shared_ptr<metapb::Region> firstRegion,
                               bool *isNewCluster) {
  return true;
}

bool RaftNode::StartNode(std::shared_ptr<DBEngines> engines,
                         std::shared_ptr<TransportInterface> trans) {
  return raftSystem_->Start(store_, raftCfg_, engines, trans);
}

void RaftNode::Stop() {}

uint64_t RaftNode::GetStoreID() { store_->id(); }

RaftNode::~RaftNode() {}

}  // namespace network
