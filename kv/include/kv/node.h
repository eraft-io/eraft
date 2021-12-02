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

#ifndef ERAFT_KV_NODE_H_
#define ERAFT_KV_NODE_H_

#include <kv/config.h>
#include <kv/engines.h>
#include <kv/raft_store.h>
#include <kv/transport.h>
#include <eraftio/metapb.pb.h>
#include <stdint.h>

#include <memory>

namespace kvserver {

class Node {
 public:
  const uint64_t kMaxCheckClusterBoostrappedRetryCount = 60;

  const uint64_t kCheckClusterBoostrapRetrySeconds = 3;

  Node(std::shared_ptr<RaftStore> system, std::shared_ptr<Config> cfg);

  bool Start(std::shared_ptr<Engines> engines, std::shared_ptr<Transport>);

  bool CheckStore(Engines& engs, uint64_t* storeId);

  bool BootstrapStore(Engines& engs, uint64_t* storeId);

  uint64_t AllocID();

  std::pair<std::shared_ptr<metapb::Region>, bool>
  CheckOrPrepareBoostrapCluster(std::shared_ptr<Engines> engines,
                                uint64_t storeID);

  bool CheckClusterBoostrapped();

  std::pair<std::shared_ptr<metapb::Region>, bool> PrepareBootstrapCluster(
      std::shared_ptr<Engines> engines, uint64_t storeID);

  bool BoostrapCluster(std::shared_ptr<Engines> engines,
                       std::shared_ptr<metapb::Region> firstRegion,
                       bool* isNewCluster);

  bool StartNode(std::shared_ptr<Engines> engines,
                 std::shared_ptr<Transport> trans);

  bool StopNode(uint64_t storeID);

  void Stop();

  uint64_t GetStoreID();

  ~Node();

  std::shared_ptr<Engines> engs_;

 private:
  uint64_t clusterID_;

  std::shared_ptr<metapb::Store> store_;

  std::shared_ptr<Config> cfg_;

  std::shared_ptr<RaftStore> system_;
};

}  // namespace kvserver

#endif