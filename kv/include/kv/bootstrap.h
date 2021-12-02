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

//
// This file define some hepler function for kvserver boot
//

#ifndef ERAFT_KV_BOOTSTRAP_H_
#define ERAFT_KV_BOOTSTRAP_H_

#include <eraftio/metapb.pb.h>
#include <kv/engines.h>
#include <rocksdb/db.h>

#include <memory>

namespace kvserver {

class BootHelper {
 public:
  // epoch version
  static const uint64_t kInitEpochVer = 1;

  // epoch config version
  static const uint64_t kInitEpochConfVer = 1;

  BootHelper(){};

  ~BootHelper(){};

  // is (startKey, endKey) empty in db
  static bool IsRangeEmpty(rocksdb::DB* db, std::string startKey,
                           std::string endKey);

  static bool DoBootstrapStore(std::shared_ptr<Engines> engines,
                               uint64_t clusterID, uint64_t storeID,
                               std::string storeAddr);

  static uint64_t MockSchAllocID();

  static std::pair<std::shared_ptr<metapb::Region>, bool> PrepareBootstrap(
      std::shared_ptr<Engines> engines, std::string storeAddr,
      std::map<std::string, int> peerAddrMaps);

  static bool PrepareBoostrapCluster(std::shared_ptr<Engines> engines,
                                     std::shared_ptr<metapb::Region> region);

  static void WriteInitialApplyState(rocksdb::WriteBatch* kvWB,
                                     uint64_t regionID);

  static void WriteInitialRaftState(rocksdb::WriteBatch* raftWB,
                                    uint64_t regionID);

  // get instance
  static BootHelper* GetInstance();

 protected:
  // single instance
  static BootHelper* instance_;

  // counter
  static uint64_t gCounter_;
};

}  // namespace kvserver

#endif  // ERAFT_KV_BOOTSTRAP_H
