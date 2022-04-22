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

#ifndef ERAFT_NETWORK_RAFT_STORE_H_
#define ERAFT_NETWORK_RAFT_STORE_H_

#include <eraftio/metapb.pb.h>
#include <eraftio/raft_messagepb.pb.h>
#include <network/raft_config.h>
#include <network/concurrency_msg_queue.h>
#include <network/transport_interface.h>
#include <network/db_engines.h>
#include <storage/write_batch.h>
#include <network/raft_stack.h>
#include <network/raft_peer.h>

#include <deque>
#include <map>
#include <memory>
#include <mutex>

namespace network
{

  class Router;

  class RaftstoreRouter;

  class RaftRouter;

  struct StoreState
  {
    uint64_t id_;
  };

  struct StoreMeta
  {
    StoreMeta()
    {
      regions_ = std::map<uint64_t, metapb::Region *>{};
      pendingVotes_ = std::vector<raft_serverpb::RaftMessage *>{};
      regionRanges_ = std::map<std::string, uint64_t>{};
    }

    std::mutex mutex_;

    std::map<std::string, uint64_t> regionRanges_;

    std::map<uint64_t, metapb::Region *> regions_;

    std::vector<raft_serverpb::RaftMessage *> pendingVotes_;
  };

  struct GlobalContext
  {
    GlobalContext(std::shared_ptr<RaftConfig> cfg, std::shared_ptr<DBEngines> engine,
                  std::shared_ptr<metapb::Store> store,
                  std::shared_ptr<StoreMeta> storeMeta,
                  std::shared_ptr<Router> router,
                  std::shared_ptr<Transport> trans)
    {
      this->cfg_ = cfg;
      this->engine_ = engine;
      this->store_ = store;
      this->storeMeta_ = storeMeta;
      this->router_ = router;
      this->trans_ = trans;
    }

    std::shared_ptr<RaftConfig> cfg_;

    std::shared_ptr<DBEngines> engine_;

    std::shared_ptr<metapb::Store> store_;

    std::shared_ptr<StoreMeta> storeMeta_;

    std::shared_ptr<Router> router_;

    std::shared_ptr<TransportInterface> trans_;
  };

  class RaftStore
  {
    friend class RaftStack;

  public:
    RaftStore();

    RaftStore(std::shared_ptr<RaftConfig> cfg);

    ~RaftStore();

    std::vector<std::shared_ptr<Peer> > LoadPeers();

    std::shared_ptr<Peer> GetLeader();

    void ClearStaleMeta(storage::WriteBatch &kvWB, storage::WriteBatch &raftWB,
                        raft_messagepb::RegionLocalState &originState);

    bool Start(std::shared_ptr<metapb::Store> meta, std::shared_ptr<RaftConfig> cfg,
               std::shared_ptr<StorageEngineInterface> engines,
               std::shared_ptr<TransportInterface> trans);

    bool StartWorkers(std::vector<std::shared_ptr<RaftPeer> > peers);

    void ShutDown();

    bool RaftStore::Write() {}

    std::string RaftStore::Reader() {}

  private:
    std::shared_ptr<GlobalContext> ctx_;

    std::shared_ptr<StoreState> state_;

    std::shared_ptr<Router> router_;

    std::shared_ptr<RaftRouter> raftRouter_;

    std::deque<uint64_t> tickDriverSender_;

    std::atomic<bool> close_;
  };

} // namespace network

#endif
