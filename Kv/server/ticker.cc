#include <Kv/ticker.h>
#include <Kv/concurrency_queue.h>
#include <Logger/Logger.h>
#include <Kv/msg.h>

namespace kvserver
{

Ticker* Ticker::instance_ = nullptr;
std::shared_ptr<Router> Ticker::router_ = nullptr;
std::map<uint64_t, void*> Ticker::regions_ = {};

Ticker::Ticker (std::function<void()> onTick, std::shared_ptr<Router> router, std::chrono::duration<int64_t, std::nano> tickInterval)
: onTick_ (onTick)
, tickInterval_(tickInterval)
, running_(false) {}

Ticker::~Ticker() {}

void Ticker::Start()
{
    if (running_)
    {
        return;
    }
    running_ = true;
    std::thread runT(&Ticker::TimerLoop, this);
    runT.detach();
}

void Ticker::Stop()
{
    running_ = false;
}

void Ticker::Run()
{
    for(auto r : regions_) 
    {
       auto msg = NewPeerMsg(MsgType::MsgTypeTick, r.first, nullptr);
       router_->Send(r.first, msg);
    }
    auto regionId = QueueContext::GetInstance()->regionIdCh_.Pop();
    regions_.insert(std::pair<uint64_t, void*>(regionId, nullptr));
}

void Ticker::SetDuration(std::chrono::duration<int64_t, std::nano> tickInterval)
{
    tickIntervalMutex_.lock();
    tickInterval_ = tickInterval;
    tickIntervalMutex_.unlock();
}

void Ticker::TimerLoop()
{
    while (running_)
    {
        std::thread run(onTick_);
        run.detach();
        tickIntervalMutex_.lock();
        std::chrono::duration<int64_t, std::nano> tickInterval = tickInterval_;
        tickIntervalMutex_.unlock();
        std::this_thread::sleep_for(tickInterval);
    }
}

    
} // namespace kvserver
