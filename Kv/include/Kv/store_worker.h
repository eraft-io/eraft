#ifndef ERAFT_KV_STORE_WORKER_H_
#define ERAFT_KV_STORE_WORKER_H_

#include <stdint.h>
#include <atomic>
#include <mutex>
#include <memory>

#include <eraftio/raft_serverpb.pb.h>

#include <Kv/msg.h>

namespace kvserver
{

using StoreTick = int;

class StoreWorker
{

public:

    StoreWorker(/* args */);
    ~StoreWorker();


    bool IsAlive() const { return running_; }
    
    void Stop() { running_ = false; }
    
    void Run();

    void OnTick(StoreTick tick);

    void HandleMsg(Msg msg);

    void Start(std::shared_ptr<metapb::Store> store);

    bool CheckMsg(std::shared_ptr<raft_serverpb::RaftMessage> msg);

    bool OnRaftMessage(std::shared_ptr<raft_serverpb::RaftMessage> msg);

    bool MaybeCreatePeer(uint64_t regionID, std::shared_ptr<raft_serverpb::RaftMessage> msg);

    void StoreHeartbeatScheduler();

    void OnSchedulerStoreHeartbeatTick();

    bool HandleSnapMgrGC();

    bool ScheduleGCSnap();  //  TODO:


protected:

private:
    uint64_t id_;
    std::atomic<bool> running_;
    std::mutex mutex_;
};


} // namespace kvserver

#endif
