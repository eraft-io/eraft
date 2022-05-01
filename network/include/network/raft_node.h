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

#ifndef ERAFT_NETWORK_RAFT_NODE_H_
#define ERAFT_NETWORK_RAFT_NODE_H_

#include <eraftio/metapb.pb.h>
#include <network/db_engines.h>
#include <network/raft_config.h>
#include <network/raft_server_transport.h>
#include <network/raft_store.h>
#include <storage/engine_interface.h>

#include <cstdint>
#include <memory>

namespace network {

class RaftStore;

class RaftConfig;

class RaftNode {
 public:
  RaftNode(std::shared_ptr<RaftStore> system, std::shared_ptr<RaftConfig> cfg);

  bool Start(std::shared_ptr<DBEngines> engines,
             std::shared_ptr<TransportInterface> trans);

  bool CheckStore(DBEngines &engs, uint64_t *storeId);

  bool BootstrapStore(DBEngines &engs, uint64_t *storeId);

  uint64_t AllocID();

  std::pair<std::shared_ptr<metapb::Region>, bool>
  CheckOrPrepareBoostrapCluster(std::shared_ptr<DBEngines> engines,
                                uint64_t storeId);

  bool CheckClusterBoostrapped();

  std::pair<std::shared_ptr<metapb::Region>, bool> PrepareBootstrapCluster(
      std::shared_ptr<DBEngines> engines, uint64_t storeId);

  bool BoostrapCluster(std::shared_ptr<DBEngines> engines,
                       std::shared_ptr<metapb::Region> firstRegion,
                       bool *isNewCluster);

  bool StartNode(std::shared_ptr<DBEngines> engines,
                 std::shared_ptr<TransportInterface> trans);

  void Stop();

  uint64_t GetStoreID();

  ~RaftNode();

 private:
  std::shared_ptr<DBEngines> engs_;

  uint64_t clusterID_;

  std::shared_ptr<metapb::Store> store_;

  std::shared_ptr<RaftConfig> raftCfg_;

  std::shared_ptr<RaftStore> raftSystem_;
};

}  // namespace network

#endif