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
#include <Kv/concurrency_queue.h>

namespace kvserver
{

using StoreTick = int;

class StoreWorker
{

public:

    StoreWorker(std::shared_ptr<GlobalContext> ctx, std::shared_ptr<StoreState> state);
    ~StoreWorker();

    static bool IsAlive() { return running_; }
    
    static void Stop() { running_ = false; }
    
    static void Run(Queue<Msg>& qu);

    static void BootThread();

    static void OnTick(StoreTick* tick);

    static void HandleMsg(Msg msg);

    static void Start(metapb::Store* store);

    static bool CheckMsg(std::shared_ptr<raft_serverpb::RaftMessage> msg);

    static bool OnRaftMessage(raft_serverpb::RaftMessage* msg);

    static bool MaybeCreatePeer(uint64_t regionID, std::shared_ptr<raft_serverpb::RaftMessage> msg);

    static void StoreHeartbeatScheduler();

    static void OnSchedulerStoreHeartbeatTick();

    static bool HandleSnapMgrGC();

    static bool ScheduleGCSnap();  //  TODO:


protected:

private:

    static uint64_t id_;

    static bool running_;

    // StoreState
    static std::shared_ptr<StoreState> storeState_;

    static std::shared_ptr<GlobalContext> ctx_;
};


} // namespace kvserver

#endif
