#ifndef ERAFT_KV_STORE_WORKER_H_
#define ERAFT_KV_STORE_WORKER_H_

#include <stdint.h>
#include <atomic>
#include <mutex>

namespace kvserver
{

class StoreWorker
{
public:
    StoreWorker(/* args */);
    ~StoreWorker();

    bool IsAlive() const { return running_; }
    
    void Stop() { running_ = false; }
    void Run();

protected:

private:
    uint64_t id_;
    std::atomic<bool> running_;
    std::mutex mutex_;
};


} // namespace kvserver

#endif
