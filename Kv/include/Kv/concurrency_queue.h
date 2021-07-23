#ifndef ERAFT_KV_QUEUE_H_
#define ERAFT_KV_QUEUE_H_

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stdint.h>
#include <Kv/msg.h>

// https://github.com/juanchopanza/cppblog/blob/master/Concurrency/Queue/Queue.h

namespace kvserver
{

template <typename T>
class Queue
{

public:

    T Pop()
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty())
        {
            cond_.wait(mlock);
        }
        auto val = queue_.front();
        queue_.pop();
        return val;
    }

    void Pop(T& item)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty())
        {
            cond_.wait(mlock);
        }
        item = queue_.front();
        queue_.pop();
    }

    void Push(const T& item)
    {
        std::unique_lock<std::mutex> mlock(mutex_);
        queue_.push(item);
        mlock.unlock();
        cond_.notify_one();
    }

    uint64_t Size()
    {

    }

    Queue() = default;

    Queue(const Queue&) = delete;            // disable copying
    Queue& operator=(const Queue&) = delete; // disable assignment


private:

    std::queue<T> queue_;

    std::mutex mutex_;

    std::condition_variable cond_;

};

class QueueContext
{

protected:

    static QueueContext* instance_;

public:

    static QueueContext* GetInstance()
    {
        if(instance_ == nullptr)
        {
            instance_ = new QueueContext();
            return instance_;
        }
    }

    QueueContext() {};

    ~QueueContext() {};

    Queue<Msg> peerSender_;

    Queue<Msg> storeSender_;

};


} // namespace kvserver


#endif