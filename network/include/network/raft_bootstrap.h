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

#ifndef ERAFT_NETWORK_RAFT_BOOTSTRAP_H_
#define ERAFT_NETWORK_RAFT_BOOTSTRAP_H_

#include <eraftio/metapb.pb.h>
#include <network/db_engines.h>
#include <storage/engine_interface.h>
#include <storage/write_batch.h>

#include <string>

namespace network {

class BootHepler {
 public:
  BootHepler();
  ~BootHepler();

  static bool IsRangeEmpty(std::shared_ptr<storage::StorageEngineInterface> db,
                           std::string startKey, std::string endKey);

  static bool DoBootstrapStore(std::shared_ptr<DBEngines> engines,
                               uint64_t clusterId, uint64_t storeId,
                               std::string storeAddr);

  static uint64_t AllocID();

  static std::pair<std::shared_ptr<metapb::Region>, bool> PrepareBootstrap(
      std::shared_ptr<DBEngines> engines, std::string storeAddr,
      std::map<std::string, int> peerAddrMaps);

  static bool PrepareBoostrapCluster(std::shared_ptr<DBEngines> engines,
                                     std::shared_ptr<metapb::Region> region);

  static void WriteInitialApplyState(storage::WriteBatch &kvWB,
                                     uint64_t regionId);

  static void WriteInitialRaftState(storage::WriteBatch &raftWB,
                                    uint64_t regionId);

  static BootHepler *GetInstance();

  static const uint64_t kInitEpochVer = 1;

  static const uint64_t kInitEpoceConfVer = 1;

 protected:
  static BootHepler *instance_;

  static uint64_t gCounter_;
};

}  // namespace network

#endif