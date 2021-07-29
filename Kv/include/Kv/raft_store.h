// MIT License

// Copyright (c) 2021 Colin

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

#ifndef ERAFT_KV_RAFTSTORE_H_
#define ERAFT_KV_RAFTSTORE_H_

#include <Kv/config.h>
#include <Kv/engines.h>
#include <Kv/msg.h>
#include <Kv/peer.h>
#include <Kv/router.h>
#include <Kv/server_transport.h>
#include <Kv/storage.h>
#include <Kv/transport.h>
#include <eraftio/metapb.pb.h>
#include <eraftio/raft_serverpb.pb.h>

#include <deque>
#include <map>
#include <memory>
#include <mutex>

namespace kvserver {

class Router;

class RaftstoreRouter;

class RaftRouter;

struct StoreState {
  uint64_t id_;
};

struct StoreMeta {
  StoreMeta() {
    regions_ = std::map<uint64_t, metapb::Region*>{};
    pendingVotes_ = std::vector<raft_serverpb::RaftMessage*>{};
    regionRanges_ = std::map<std::string, uint64_t>{};
  }

  std::mutex mutex_;

  // region end key -> region id
  std::map<std::string, uint64_t> regionRanges_;

  std::map<uint64_t, metapb::Region*> regions_;

  std::vector<raft_serverpb::RaftMessage*> pendingVotes_;
};

struct GlobalContext {
  GlobalContext(std::shared_ptr<Config> cfg, std::shared_ptr<Engines> engine,
                std::shared_ptr<metapb::Store> store,
                std::shared_ptr<StoreMeta> storeMeta,
                std::shared_ptr<Router> router,
                std::shared_ptr<Transport> trans) {
    this->cfg_ = cfg;
    this->engine_ = engine;
    this->store_ = store;
    this->storeMeta_ = storeMeta;
    this->router_ = router;
    this->trans_ = trans;
  }

  std::shared_ptr<Config> cfg_;

  std::shared_ptr<Engines> engine_;

  std::shared_ptr<metapb::Store> store_;

  std::shared_ptr<StoreMeta> storeMeta_;

  std::shared_ptr<Router> router_;

  std::shared_ptr<Transport> trans_;

  // TODO: Scheduler Client
};

class RaftStore {
  friend class RaftStorage;

 public:
  RaftStore();

  RaftStore(std::shared_ptr<Config> cfg);

  ~RaftStore();

  std::vector<std::shared_ptr<Peer> > LoadPeers();

  void ClearStaleMeta(rocksdb::WriteBatch* kvWB, rocksdb::WriteBatch* raftWB,
                      raft_serverpb::RegionLocalState* originState);

  bool Start(std::shared_ptr<metapb::Store> meta, std::shared_ptr<Config> cfg,
             std::shared_ptr<Engines> engines,
             std::shared_ptr<Transport> trans);

  bool StartWorkers(std::vector<std::shared_ptr<Peer> > peers);

  void ShutDown();

  bool Write(kvrpcpb::Context* ctx, std::vector<Modify>);

  StorageReader* Reader(kvrpcpb::Context* ctx);

 private:
  std::shared_ptr<GlobalContext> ctx_;

  std::shared_ptr<StoreState> state_;

  std::shared_ptr<Router> router_;

  std::shared_ptr<RaftRouter> raftRouter_;

  // scheduler client: TODO
  std::deque<uint64_t> tickDriverSender_;

  std::atomic<bool> close_;
};

}  // namespace kvserver

#endif