// MIT License

// Copyright (c) 2021 eraft dev group

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

#ifndef ERAFT_THREAD_POOL_H_
#define ERAFT_THREAD_POOL_H_

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>

class ThreadPool final {
 public:
  ~ThreadPool();

  ThreadPool(const ThreadPool&) = delete;
  void operator=(const ThreadPool&) = delete;

  static ThreadPool& Instance();

  template <typename F, typename... Args>
  auto ExecuteTask(F&& f, Args&&... args)
      -> std::future<typename std::result_of<F(Args...)>::type>;

  void JoinAll();
  void SetMaxIdleThread(unsigned int m);

 private:
  ThreadPool();

  void _CreateWorker();
  void _WorkerRoutine();
  void _MonitorRoutine();

  std::thread monitor_;
  std::atomic<unsigned> maxIdleThread_;
  std::atomic<unsigned> pendingStopSignal_;

  static __thread bool working_;
  std::deque<std::thread> workers_;

  std::mutex mutex_;
  std::condition_variable cond_;
  unsigned waiters_;
  bool shutdown_;
  std::deque<std::function<void()> > tasks_;

  static const int kMaxThreads = 256;
};

template <typename F, typename... Args>
auto ThreadPool::ExecuteTask(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {
  using resultType = typename std::result_of<F(Args...)>::type;

  auto task = std::make_shared<std::packaged_task<resultType()> >(
      std::bind(std::forward<F>(f), std::forward<Args>(args)...));

  {
    std::unique_lock<std::mutex> guard(mutex_);
    if (shutdown_) {
      return std::future<resultType>();
    }

    tasks_.emplace_back([=]() { (*task)(); });
    if (waiters_ == 0) {
      _CreateWorker();
    }

    cond_.notify_one();
  }

  return task->get_future();
}

#endif