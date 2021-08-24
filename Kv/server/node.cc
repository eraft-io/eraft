// MIT License

// Copyright (c) 2021 eraft dev group

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

#include <Kv/bootstrap.h>
#include <Kv/node.h>
#include <Kv/utils.h>
#include <eraftio/raft_serverpb.pb.h>
#include <spdlog/spdlog.h>

namespace kvserver {

extern bool DoBootstrapStore(std::shared_ptr<Engines> engines,
                             uint64_t clusterID, uint64_t storeID);

extern uint64_t gCounter;

Node::Node(std::shared_ptr<RaftStore> system, std::shared_ptr<Config> cfg)
    : clusterID_(1), cfg_(cfg), system_(system) {
  metapb::Store store;
  store.set_address(cfg->storeAddr_);
  this->store_ = std::make_shared<metapb::Store>(store);
}

Node::~Node() {}

bool Node::Start(std::shared_ptr<Engines> engines,
                 std::shared_ptr<Transport> trans) {
  uint64_t storeID;
  if (!this->CheckStore(*engines, &storeID)) {
    SPDLOG_INFO("store id " + std::to_string(storeID) + " not found");
  }
  if (storeID == Assistant::GetInstance()->kInvalidID) {
    this->BootstrapStore(*engines, &storeID);
  }
  this->store_->set_id(storeID);
  auto checkRes = this->CheckOrPrepareBoostrapCluster(engines, storeID);
  if (!checkRes.second) {
    SPDLOG_ERROR("check or prepare boostrap cluster " +
                 std::to_string(storeID) + " not found");
    return false;
  }
  bool newCluster = (checkRes.first != nullptr);
  if (newCluster) {
    // try to boostrap cluster
    if (!this->BoostrapCluster(engines, checkRes.first, &newCluster)) {
      return false;
    }
  }
  if (!this->StartNode(engines, trans)) {
    return false;
  }
  return true;
}

bool Node::CheckStore(Engines& engs, uint64_t* storeId) {
  raft_serverpb::StoreIdent ident;
  if (!Assistant::GetInstance()
           ->GetMeta(engs.kvDB_,
                     Assistant::GetInstance()->VecToString(
                         Assistant::GetInstance()->StoreIdentKey),
                     &ident)
           .ok()) {
    *storeId = 0;
  }
  if (ident.cluster_id() != this->clusterID_) {
    *storeId = 0;
    return false;
  }
  if (ident.store_id() == Assistant::GetInstance()->kInvalidID) {
    *storeId = 0;
    return false;
  }
  *storeId = ident.store_id();
  return true;
}

uint64_t Node::AllocID() {
  return BootHelper().GetInstance()->MockSchAllocID();
}

std::pair<std::shared_ptr<metapb::Region>, bool>
Node::CheckOrPrepareBoostrapCluster(std::shared_ptr<Engines> engines,
                                    uint64_t storeID) {
  raft_serverpb::RegionLocalState* state =
      new raft_serverpb::RegionLocalState();
  if (Assistant::GetInstance()
          ->GetMeta(engines->kvDB_,
                    Assistant::GetInstance()->VecToString(
                        Assistant::GetInstance()->PrepareBootstrapKey),
                    state)
          .ok()) {
    return std::make_pair<std::shared_ptr<metapb::Region>, bool>(
        std::make_shared<metapb::Region>(state->region()), true);
  }
  delete state;
  return this->PrepareBootstrapCluster(engines, storeID);
}

bool Node::CheckClusterBoostrapped() { return false; }

std::pair<std::shared_ptr<metapb::Region>, bool> Node::PrepareBootstrapCluster(
    std::shared_ptr<Engines> engines, uint64_t storeID) {
  return BootHelper().GetInstance()->PrepareBootstrap(
      engines, this->store_->address(), this->cfg_->peerAddrMaps_);
}

bool Node::BoostrapCluster(std::shared_ptr<Engines> engines,
                           std::shared_ptr<metapb::Region> firstRegion,
                           bool* isNewCluster) {
  auto regionID = firstRegion->id();
  // TODO: report to center
  return true;
}

bool Node::BootstrapStore(Engines& engs, uint64_t* storeId) {
  auto storeID = this->cfg_->peerAddrMaps_[this->cfg_->storeAddr_];
  SPDLOG_INFO("boostrap store with storeID: " + std::to_string(storeID));
  if (!BootHelper::GetInstance()->DoBootstrapStore(
          std::make_shared<Engines>(engs), this->clusterID_, storeID,
          this->store_->address())) {
    SPDLOG_ERROR("do bootstrap store error!");
    return false;
  }
  *storeId = storeID;
  return true;
}

bool Node::StartNode(std::shared_ptr<Engines> engines,
                     std::shared_ptr<Transport> trans) {
  return this->system_->Start(this->store_, this->cfg_, engines, trans);
}

bool Node::StopNode(uint64_t storeID) { this->system_->ShutDown(); }

void Node::Stop() { this->StopNode(this->store_->id()); }

uint64_t Node::GetStoreID() { return this->store_->id(); }

}  // namespace kvserver
