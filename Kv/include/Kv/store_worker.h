#ifndef ERAFT_KV_STORE_WORKER_H_
#define ERAFT_KV_STORE_WORKER_H_

#include <stdint.h>
#include <atomic>
#include <mutex>
#include <memory>
#include <thread>

#include <eraftio/raft_serverpb.pb.h>

#include <Kv/msg.h>
#include <Kv/raft_store.h>

namespace kvserver
{

using StoreTick = int;

class StoreWorker
{

public:

    StoreWorker(std::shared_ptr<GlobalContext> ctx, std::shared_ptr<StoreState> state);
    ~StoreWorker();

    bool IsAlive() const { return running_; }
    
    void Stop() { running_ = false; }
    
    void Run();

    void BootThread();

    void OnTick(StoreTick* tick);

    void HandleMsg(Msg msg);

    void Start(metapb::Store* store);

    bool CheckMsg(std::shared_ptr<raft_serverpb::RaftMessage> msg);

    bool OnRaftMessage(raft_serverpb::RaftMessage* msg);

    bool MaybeCreatePeer(uint64_t regionID, std::shared_ptr<raft_serverpb::RaftMessage> msg);

    void StoreHeartbeatScheduler();

    void OnSchedulerStoreHeartbeatTick();

    bool HandleSnapMgrGC();

    bool ScheduleGCSnap();  //  TODO:


protected:

private:

    uint64_t id_;

    bool running_;

    // StoreState
    std::shared_ptr<StoreState> storeState_;

    std::shared_ptr<GlobalContext> ctx_;
};


} // namespace kvserver

#endif
