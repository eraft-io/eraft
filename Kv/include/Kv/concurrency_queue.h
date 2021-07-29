// MIT License

// Copyright (c) 2021 Colin

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ERAFT_KV_CONCURRENCY_QUEUE_H_
#define ERAFT_KV_CONCURRENCY_QUEUE_H_

#include <Kv/msg.h>
#include <stdint.h>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

namespace kvserver {

template <typename T>
class Queue {
 public:
  T Pop() {
    std::unique_lock<std::mutex> mlock(mutex_);
    while (queue_.empty()) {
      cond_.wait(mlock);
    }
    auto val = queue_.front();
    queue_.pop();
    return val;
  }

  void Pop(T& item) {
    std::unique_lock<std::mutex> mlock(mutex_);
    while (queue_.empty()) {
      cond_.wait(mlock);
    }
    item = queue_.front();
    queue_.pop();
  }

  void Push(const T& item) {
    std::unique_lock<std::mutex> mlock(mutex_);
    queue_.push(item);
    mlock.unlock();
    cond_.notify_one();
  }

  uint64_t Size() {}

  Queue() = default;

  Queue(const Queue&) = delete;             // disable copying
  Queue& operator=(const Queue&) = delete;  // disable assignment

 private:
  std::queue<T> queue_;

  std::mutex mutex_;

  std::condition_variable cond_;
};

class QueueContext {
 public:
  QueueContext(){};

  ~QueueContext(){};

  static QueueContext* GetInstance() {
    if (instance_ == nullptr) {
      instance_ = new QueueContext();
      return instance_;
    }
  }

  Queue<Msg> peerSender_;

  Queue<Msg> storeSender_;

  Queue<uint64_t> regionIdCh_;

 protected:
  static QueueContext* instance_;
};

}  // namespace kvserver

#endif