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
#include <network/raft_encode_assistant.h>
#include <network/raft_store.h>

namespace network {

RaftStore::RaftStore() {
  router_ = std::make_shared<Router>();
  raftRouter_ = std::make_shared<RaftStoreRouter>(router_);
  SPDLOG_INFO("raft store init success!");
}

RaftStore::~RaftStore() {}

std::shared_ptr<GlobalContext> RaftStore::GetContext() { return ctx_; }

std::vector<std::shared_ptr<RaftPeer> > RaftStore::LoadPeers() {
  auto ctx = this->ctx_;
  auto storeID = ctx->store_->id();
  SPDLOG_INFO("raft store store id: " + std::to_string(storeID) +
              " load peers");
  std::vector<std::shared_ptr<RaftPeer> > regionPeers;

  std::vector<std::string> keys;
  std::vector<std::string> values;
  ctx->engine_->kvDB_->RangeQuery(
      RaftEncodeAssistant::GetInstance()->RegionMetaMinKeyStr(),
      RaftEncodeAssistant::GetInstance()->RegionMetaMaxKeyStr(), keys, values);
  for (int i = 0; i < keys.size(); i++) {
    uint64_t regionID;
    uint8_t suffix;

    RaftEncodeAssistant::GetInstance()->DecodeRegionMetaKey(
        RaftEncodeAssistant::GetInstance()->StringToVec(keys[i]), &regionID,
        &suffix);

    if (suffix != RaftEncodeAssistant::GetInstance()->kRegionStateSuffix[0]) {
      continue;
    }

    raft_messagepb::RegionLocalState *localState =
        new raft_messagepb::RegionLocalState();
    localState->ParseFromString(values[i]);

    auto region = localState->region();
    metapb::Region *region1 = new metapb::Region(region);

    // TODO: clear stale meta
    std::shared_ptr<RaftPeer> peer =
        std::make_shared<RaftPeer>(storeID, ctx->cfg_, ctx->engine_,
                                   std::make_shared<metapb::Region>(region));
    // ctx->storeMeta_->regions_[regionID] = &region;
    ctx->storeMeta_->regions_.insert(
        std::pair<int, metapb::Region *>(regionID, region1));
    regionPeers.push_back(peer);
  }

  return regionPeers;
}

std::shared_ptr<RaftPeer> RaftStore::GetLeader() {
  auto regionPeers = this->LoadPeers();
  for (auto peer : regionPeers) {
    if (peer->PeerId() == peer->LeaderId()) {
      return peer;
    }
  }
}

void RaftStore::ClearStaleMeta(storage::WriteBatch &kvWB,
                               storage::WriteBatch &raftWB,
                               raft_messagepb::RegionLocalState &originState) {}

bool RaftStore::Start(std::shared_ptr<metapb::Store> meta,
                      std::shared_ptr<RaftConfig> cfg,
                      std::shared_ptr<DBEngines> engines,
                      std::shared_ptr<TransportInterface> trans) {
  assert(cfg->Validate());

  std::shared_ptr<StoreMeta> storeMeta = std::make_shared<StoreMeta>();

  this->ctx_ = std::make_shared<GlobalContext>(cfg, engines, meta, storeMeta,
                                               this->router_, trans);

  // register peer
  auto regionPeers = this->LoadPeers();

  for (auto peer : regionPeers) {
    this->router_->Register(peer);
  }

  this->StartWorkers(regionPeers);
}

bool RaftStore::StartWorkers(std::vector<std::shared_ptr<RaftPeer> > peers) {
  auto ctx = this->ctx_;
  auto router = this->router_;
  auto state = this->state_;
  auto rw = RaftWorker(ctx, router);
  rw.BootThread();
  Msg m(MsgType::MsgTypeStoreStart, ctx->store_.get());
  router->SendStore(m);
  for (uint64_t i = 0; i < peers.size(); i++) {
    auto regionID = peers[i]->regionId_;
    Msg m(MsgType::MsgTypeStart, ctx->store_.get());
    router->Send(regionID, m);
  }
  // ticker start
  std::chrono::duration<int, std::milli> timer_tick(50);
  Ticker::GetInstance(std::function<void()>(Ticker::Run), router, timer_tick)
      ->Start();

  return true;
}

void RaftStore::ShutDown() {}

}  // namespace network
