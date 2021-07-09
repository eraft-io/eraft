#include <Kv/ticker.h>

namespace kvserver
{

Ticker::Ticker (std::function<void()> onTick, std::chrono::duration<int64_t, std::nano> tickInterval)
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
