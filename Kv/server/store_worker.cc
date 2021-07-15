#include <Kv/store_worker.h>

namespace kvserver
{

StoreWorker::StoreWorker(std::shared_ptr<GlobalContext> ctx, std::shared_ptr<StoreState> state)
{
    this->storeState_ = state;
    this->ctx_ = ctx;
}

StoreWorker::~StoreWorker()
{

}

void StoreWorker::BootThread()
{
    std::thread th(&StoreWorker::Run, this);
    th.detach();
}

void StoreWorker::Run() {
    while (IsAlive())
    {
        while (this->storeState_->receiver_.size() > 0)
        {
            Msg m = *this->storeState_->receiver_.end();
            this->HandleMsg(m);
            this->storeState_->receiver_.pop_back();
        }
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
            if(!this->OnRaftMessage(static_cast<raft_serverpb::RaftMessage*>(msg.data_)))
            {
                // TODO: log handle raft message failed store id
            }
            break;
        }
    case MsgType::MsgTypeStoreTick:
        {
            this->OnTick(static_cast<StoreTick*>(msg.data_));
            break;
        }
    case MsgType::MsgTypeStoreStart:
        {
            this->Start(static_cast<metapb::Store*>(msg.data_));
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
