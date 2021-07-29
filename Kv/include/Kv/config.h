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

#ifndef ERAFT_KV_CONFIG_H_
#define ERAFT_KV_CONFIG_H_

#include <Logger/Logger.h>
#include <RaftCore/Util.h>
#include <stdint.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace kvserver {

const static uint64_t KB = 1024;

const static uint64_t MB = 1024 * 1024;

struct Config {
  // default config
  Config(std::string storeAddr, std::string dbPath, int id) {
    this->schedulerAddr_ = "127.0.0.1:2379";
    this->storeAddr_ = storeAddr;
    this->logLevel_ = "info";
    this->raft_ = true;
    this->raftBaseTickInterval_ = 1;       // 1s
    this->raftHeartbeatTicks_ = 2;         // 2s
    this->raftElectionTimeoutTicks_ = 10;  // 10s
    this->raftLogGCTickInterval_ = 10;     // 10s
    this->raftLogGcCountLimit_ = 128000;
    this->splitRegionCheckTickInterval_ = 10;         // 10s
    this->schedulerHeartbeatTickInterval_ = 100;      // 100ms
    this->schedulerStoreHeartbeatTickInterval_ = 10;  // 10s
    this->regionMaxSize_ = 144 * MB;
    this->regionSplitSize_ = 96 * MB;
    this->dbPath_ = dbPath;
    this->peerAddrMaps_ = {
        {"127.0.0.1:20160", 1}, {"127.0.0.1:20161", 2}, {"127.0.0.1:20162", 3}};
  }

  Config(std::string storeAddr, bool raft, std::string schedulerAddr,
         std::string logLevel, std::string dbPath,
         uint64_t raftBaseTickInterval, uint64_t raftHeartbeatTicks,
         uint64_t raftElectionTimeoutTicks, uint64_t raftLogGCTickInterval,
         uint64_t raftLogGcCountLimit, uint64_t splitRegionCheckTickInterval,
         uint64_t schedulerHeartbeatTickInterval,
         uint64_t schedulerStoreHeartbeatTickInterval, uint64_t regionMaxSize,
         uint64_t regionSplitSize)
      : storeAddr_(storeAddr),
        raft_(raft),
        schedulerAddr_(schedulerAddr),
        logLevel_(logLevel),
        dbPath_(dbPath),
        raftBaseTickInterval_(raftBaseTickInterval),
        raftHeartbeatTicks_(raftHeartbeatTicks),
        raftElectionTimeoutTicks_(raftElectionTimeoutTicks),
        raftLogGCTickInterval_(raftLogGCTickInterval),
        raftLogGcCountLimit_(raftLogGcCountLimit),
        splitRegionCheckTickInterval_(splitRegionCheckTickInterval),
        schedulerHeartbeatTickInterval_(schedulerHeartbeatTickInterval),
        schedulerStoreHeartbeatTickInterval_(
            schedulerStoreHeartbeatTickInterval),
        regionMaxSize_(regionMaxSize),
        regionSplitSize_(regionSplitSize) {}

  void PrintConfigToConsole() {
    std::string output =
        "\n Current StoreConfig: \n { schedulerAddr_: " + this->schedulerAddr_ +
        " \n " + " storeAddr_: " + this->storeAddr_ + " \n " +
        " logLevel_: " + this->logLevel_ + " \n " +
        " raft_: " + eraft::BoolToString(this->raft_) + " \n " +
        " raftBaseTickInterval_: " +
        std::to_string(this->raftBaseTickInterval_) + " \n " +
        " raftHeartbeatTicks_: " + std::to_string(this->raftHeartbeatTicks_) +
        " \n " + " raftElectionTimeoutTicks_: " +
        std::to_string(this->raftElectionTimeoutTicks_) + " \n " +
        " raftLogGCTickInterval_: " +
        std::to_string(this->raftLogGCTickInterval_) + " \n " +
        " raftLogGcCountLimit_: " + std::to_string(this->raftLogGcCountLimit_) +
        " \n " + " splitRegionCheckTickInterval_: " +
        std::to_string(this->splitRegionCheckTickInterval_) + " \n " +
        " schedulerHeartbeatTickInterval_: " +
        std::to_string(this->schedulerHeartbeatTickInterval_) + " \n " +
        " schedulerStoreHeartbeatTickInterval_: " +
        std::to_string(this->schedulerStoreHeartbeatTickInterval_) + " \n " +
        " regionMaxSize_: " + std::to_string(this->regionMaxSize_) + " \n " +
        " regionSplitSize_: " + std::to_string(this->regionSplitSize_) +
        " \n " + " dbPath_: " + this->dbPath_ + "\n" + " } ";

    Logger::GetInstance()->DEBUG_NEW(output, __FILE__, __LINE__,
                                     "PrintConfigToConsole");
  }

  bool Validate() {
    if (this->raftHeartbeatTicks_ == 0) {
      return false;
    }
    if (this->raftElectionTimeoutTicks_ != 10) {
      // TODO: log warn
    }
    if (this->raftElectionTimeoutTicks_ <= this->raftHeartbeatTicks_) {
      // election tick must be greater than heartbeat tick
      return false;
    }
  }

  std::string storeAddr_;

  bool raft_;

  std::map<std::string, int> peerAddrMaps_;

  std::string schedulerAddr_;

  std::string logLevel_;

  std::string dbPath_;

  uint64_t raftBaseTickInterval_;

  uint64_t raftHeartbeatTicks_;

  uint64_t raftElectionTimeoutTicks_;

  uint64_t raftLogGCTickInterval_;

  uint64_t raftLogGcCountLimit_;

  uint64_t splitRegionCheckTickInterval_;

  uint64_t schedulerHeartbeatTickInterval_;

  uint64_t schedulerStoreHeartbeatTickInterval_;

  uint64_t regionMaxSize_;

  uint64_t regionSplitSize_;
};

}  // namespace kvserver

#endif