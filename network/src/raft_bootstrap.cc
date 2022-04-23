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

#include <eraftio/raft_messagepb.pb.h>
#include <network/raft_bootstrap.h>
#include <network/raft_encode_assistant.h>
#include <spdlog/spdlog.h>

#include <memory>

namespace network {

BootHepler *BootHepler::instance_ = nullptr;

uint64_t BootHepler::gCounter_ = 0;

BootHepler::BootHepler() {}

BootHepler::~BootHepler() {}

bool BootHepler::IsRangeEmpty(std::shared_ptr<StorageEngineInterface> db,
                              std::string startKey, std::string endKey) {
  // TODO: checkout is range empty in db
  return true;
}

//
// Save clusterId and storeId to storage engine
//
// Key: StoreIdent {0x01, 0x02}
// Value:
// message StoreIdent {
//     uint64 cluster_id = 1;
//     uint64 store_id = 2;
//     string addr = 3;
// }
//

bool BootHepler::DoBootstrapStore(std::shared_ptr<DBEngines> engines,
                                  uint64_t clusterId, uint64_t storeId,
                                  std::string storeAddr) {
  std::shared_ptr<raft_messagepb::StoreIdent> storeIdent(
      new raft_messagepb::StoreIdent());
  if (!IsRangeEmpty(engines->kvDB_, "",
                    RaftEncodeAssistant::GetInstance()->MaxKeyStr()))) {
      SPDLOG_ERROR("kv db is not empty");
      return false;
    }

  if (!IsRangeEmpty(engines->raftDB_, "",
                    RaftEncodeAssistant::GetInstance()->MaxKeyStr())) {
  }

  storeIdent->set_cluster_id(clusterId);
  storeIdent->set_store_id(storeId);

  return RaftEncodeAssistant::GetInstance()->PutMessageToEngine(
      engines->kvDB_, RaftEncodeAssistant::GetInstance()->StoreIdentKeyStr(),
      *storeIdent);
}

//
// Aolloc ID for ?
//
uint64_t BootHepler::AllocID() {
  gCounter_++;
  return gCounter_;
}

std::pair<std::shared_ptr<metapb::Region>, bool> BootHepler::PrepareBootstrap(
    std::shared_ptr<DBEngines> engines, std::string storeAddr,
    std::map<std::string, int> peerAddrMaps) {
  std::shared_ptr<metapb::Region> region =
      std::make::make_shared<metapb::Region>();
  for (auto item : peerAddrMaps) {
    if (item.first == storeAddr) {
    }
  }
}

bool BootHepler::PrepareBoostrapCluster(
    std::shared_ptr<DBEngines> engines,
    std::shared_ptr<metapb::Region> region) {}

void BootHepler::WriteInitialApplyState(storage::WriteBatch &kvWB,
                                        uint64_t regionId) {
  std::shared_ptr<raft_serverpb::RaftApplyState> applyState =
      std::make_shared<raft_serverpb::RaftApplyState>();
  applyState->set_index(RaftEncodeAssistant::GetInstance()->kRaftInitLogIndex);
  applyState->set_applied_index(
      RaftEncodeAssistant::GetInstance()->kRaftInitLogIndex);
  applyState->set_term(RaftEncodeAssistant::GetInstance()->kRaftInitLogTerm);
  return RaftEncodeAssistant::GetInstance()->PutMessageToEngine(
      engines->kvDB_, RaftEncodeAssistant::GetInstance()->ApplyStateKey(),
      *applyState);
}

// need
void BootHepler::WriteInitialRaftState(storage::WriteBatch &raftWB,
                                       uint64_t regionId) {}

BootHepler *BootHepler::GetInstance() {
  if (instance_ == nullptr) {
    instance_ = new BootHepler();
  }
  return instance_;
}

}  // namespace network
