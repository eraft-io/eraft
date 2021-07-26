#ifndef ERAFT_KV_TICKER_H_
#define ERAFT_KV_TICKER_H_

//
// Usage example
//
// void tick()
// {
//     std::cout << "tick\n";
// }

// void main()
// {
//     std::chrono::duration<int, std::milli> timer_duration1(1000);
//     std::chrono::duration<int, std::milli> timer_duration2(500);
//     std::chrono::duration<int> main_wait(5);

//     Ticker ticker(std::function<void()>(tick), timer_duration1);
//     ticker.start();

//     std::this_thread::sleep_for(main_wait);
//     ticker.setDuration(timer_duration2);
//     std::this_thread::sleep_for(main_wait);
//     ticker.stop();
// }
// 

#include <cstdint>
#include <functional>
#include <chrono>
#include <thread>
#include <future>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <memory>
#include <map>

#include <Kv/router.h>


namespace kvserver
{

class Ticker 
{

public:

    typedef std::chrono::duration<int64_t, std::nano> tick_interval_t;
    typedef std::function<void()> on_tick_t;

    Ticker(std::function<void()> onTick, std::shared_ptr<Router> router, std::chrono::duration<int64_t, std::nano> tickInterval);
    ~Ticker();

    static void Run();

    void Start();
    void Stop();

    void SetDuration(std::chrono::duration<int64_t, std::nano> tickInterval);

    void TimerLoop();

    static Ticker* GetInstance(std::function<void()> onTick, std::shared_ptr<Router> router, std::chrono::duration<int64_t, std::nano> tickInterval)
    {
        if(instance_ == nullptr)
        {
            instance_ = new Ticker(onTick, router, tickInterval);
            router_ = router;
        }
        return instance_;
    }

    static std::shared_ptr<Router> router_;

private:
    static Ticker* instance_;

    static std::map<uint64_t, void*> regions_;
    on_tick_t onTick_;
    tick_interval_t tickInterval_;
    volatile bool running_;
    std::mutex tickIntervalMutex_;

};

} // namespace kvserver


#endif