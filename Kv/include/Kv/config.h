#ifndef ERAFT_KV_CONFIG_H_
#define ERAFT_KV_CONFIG_H_

#include <string>
#include <stdint.h>
#include <iostream>

namespace kvserver
{

const static uint64_t KB = 1024;

const static uint64_t MB = 1024 * 1024;
    
struct Config
{

    std::string storeAddr_;

    bool raft_;

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

    // default config
    Config() 
    {
        this->schedulerAddr_ = "127.0.0.1:2379";
        this->storeAddr_ = "127.0.0.1:20160";
        this->logLevel_ = "info";
        this->raft_ = true;
        this->raftBaseTickInterval_ = 1; // 1s
        this->raftHeartbeatTicks_ = 2;
        this->raftElectionTimeoutTicks_ = 10;
        this->raftLogGCTickInterval_ = 10; // 10s
        this->raftLogGcCountLimit_ = 128000;
        this->splitRegionCheckTickInterval_ = 10; // 10s
        this->schedulerHeartbeatTickInterval_ = 100; // 100ms
        this->schedulerStoreHeartbeatTickInterval_ = 10; // 10s
        this->regionMaxSize_ = 144 * MB;
        this->regionSplitSize_ = 96 * MB;
        this->dbPath_ = "/tmp/eraft_data";
    }

    void PrintConfigToConsole() 
    {
          std::cout << "\n Current StoreConfig: \n { schedulerAddr_: " << this->schedulerAddr_ << " \n "
                    << " storeAddr_: " << this->storeAddr_ << " \n " 
                    << " logLevel_: " << this->logLevel_ << " \n "
                    << " raft_: " << this->raft_ << " \n "
                    << " raftBaseTickInterval_: " << this->raftBaseTickInterval_ << " \n "
                    << " raftHeartbeatTicks_: " << this->raftHeartbeatTicks_ << " \n "
                    << " raftElectionTimeoutTicks_: " << this->raftElectionTimeoutTicks_ << " \n "
                    << " raftLogGCTickInterval_: " << this->raftLogGCTickInterval_ << " \n "
                    << " raftLogGcCountLimit_: " << this->raftLogGcCountLimit_ << " \n "
                    << " splitRegionCheckTickInterval_: " << this->splitRegionCheckTickInterval_ << " \n "
                    << " schedulerHeartbeatTickInterval_: " << this->schedulerHeartbeatTickInterval_ << " \n "
                    << " schedulerStoreHeartbeatTickInterval_: " << this->schedulerStoreHeartbeatTickInterval_ << " \n "
                    << " regionMaxSize_: " << this->regionMaxSize_ << " \n "
                    << " regionSplitSize_: " << this->regionSplitSize_ << " \n "
                    << " dbPath_: " << this->dbPath_ << "\n"
                    << " } "; 
    }

    Config(std::string storeAddr, bool raft, std::string schedulerAddr, std::string logLevel, 
        std::string dbPath, uint64_t raftBaseTickInterval, uint64_t raftHeartbeatTicks, uint64_t raftElectionTimeoutTicks,
        uint64_t raftLogGCTickInterval, uint64_t raftLogGcCountLimit, uint64_t splitRegionCheckTickInterval, 
        uint64_t schedulerHeartbeatTickInterval, uint64_t schedulerStoreHeartbeatTickInterval, uint64_t regionMaxSize, 
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
      schedulerStoreHeartbeatTickInterval_(schedulerStoreHeartbeatTickInterval),
      regionMaxSize_(regionMaxSize),
      regionSplitSize_(regionSplitSize)
    {
    }

    bool Validate()
    {
        if(this->raftHeartbeatTicks_ == 0)
        {
            return false;
        }
        if(this->raftElectionTimeoutTicks_ != 10)
        {
            //TODO: log warn
        }
        if(this->raftElectionTimeoutTicks_ <= this->raftHeartbeatTicks_)
        {
            // election tick must be greater than heartbeat tick
            return false;
        }
    }

};



} // namespace kvserver


#endif