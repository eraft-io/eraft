#include <Kv/store_worker.h>
#include <Logger/Logger.h>

namespace kvserver
{

uint64_t StoreWorker::id_ = 0;

bool StoreWorker::running_ = true;

// StoreState
std::shared_ptr<StoreState> StoreWorker::storeState_ = nullptr;

std::shared_ptr<GlobalContext> StoreWorker::ctx_ = nullptr;

StoreWorker::StoreWorker(std::shared_ptr<GlobalContext> ctx, std::shared_ptr<StoreState> state)
{
    StoreWorker::storeState_ = state;
    StoreWorker::ctx_ = ctx;
}

StoreWorker::~StoreWorker()
{

}

void StoreWorker::BootThread()
{
    std::thread th(std::bind(&Run, std::ref(QueueContext::GetInstance()->storeSender_)));
    th.detach();
}

void StoreWorker::Run(Queue<Msg>& qu) {
    Logger::GetInstance()->INFO("store worker running!");
    while (IsAlive())
    {
        auto msg = qu.Pop();
        Logger::GetInstance()->INFO("pop new messsage from store sender");
        StoreWorker::HandleMsg(msg);
    }
}

void StoreWorker::OnTick(StoreTick* tick)
{

}

void StoreWorker::HandleMsg(Msg msg)
{
    switch (msg.type_)
    {
    case MsgType::MsgTypeStoreRaftMessage:
        {
            if(!StoreWorker::OnRaftMessage(static_cast<raft_serverpb::RaftMessage*>(msg.data_)))
            {
                // TODO: log handle raft message failed store id
            }
            break;
        }
    case MsgType::MsgTypeStoreTick:
        {
            StoreWorker::OnTick(static_cast<StoreTick*>(msg.data_));
            break;
        }
    case MsgType::MsgTypeStoreStart:
        {
            StoreWorker::Start(static_cast<metapb::Store*>(msg.data_));
            break;
        }
    default:
        break;
    }
}

void StoreWorker::Start(metapb::Store* store)
{

}

bool StoreWorker::CheckMsg(std::shared_ptr<raft_serverpb::RaftMessage> msg)
{

}

bool StoreWorker::OnRaftMessage(raft_serverpb::RaftMessage* msg)
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
