#include <Kv/store_worker.h>

namespace kvserver
{

StoreWorker::StoreWorker(/* args */)
{

}

StoreWorker::~StoreWorker()
{

}

void StoreWorker::Run() {
    while (IsAlive())
    {
        // receive and handle msg
    }
}

void StoreWorker::OnTick(StoreTick tick)
{

}

void StoreWorker::HandleMsg(Msg msg)
{

}

void StoreWorker::Start(std::shared_ptr<metapb::Store> store)
{

}

bool StoreWorker::CheckMsg(std::shared_ptr<raft_serverpb::RaftMessage> msg)
{

}

bool StoreWorker::OnRaftMessage(std::shared_ptr<raft_serverpb::RaftMessage> msg)
{

}

bool StoreWorker::MaybeCreatePeer(uint64_t regionID, std::shared_ptr<raft_serverpb::RaftMessage> msg)
{

}

void StoreWorker::StoreHeartbeatScheduler()
{

}

void StoreWorker::OnSchedulerStoreHeartbeatTick()
{

}

bool StoreWorker::HandleSnapMgrGC()
{

}

bool StoreWorker::ScheduleGCSnap()  //  TODO:
{

}

} // namespace kvserver
