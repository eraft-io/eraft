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

#ifndef ERAFT_NETWORK_RAFT_TICKER_H_
#define ERAFT_NETWORK_RAFT_TICKER_H_

#include <network/concurrency_msg_queue.h>
#include <network/raft_router.h>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

namespace network {
class Ticker {
 public:
  typedef std::chrono::duration<int64_t, std::nano> tick_interval_t;
  typedef std::function<void()> on_tick_t;

  Ticker(on_tick_t onTick, std::shared_ptr<Router> router,
         tick_interval_t tickInterval);
  ~Ticker();

  static void Run();

  void Start();

  void Stop();

  void SetDuration(tick_interval_t tickInterval);

  void TimerLoop();

  static Ticker *GetInstance(on_tick_t onTick, std::shared_ptr<Router> router,
                             tick_interval_t tickInterval) {
    if (instance_ == nullptr) {
      instance_ = new Ticker(onTick, router, tickInterval);
    }
    return instance_;
  }

 private:
  static Ticker *instance_;

  static std::shared_ptr<Router> router_;

  static std::map<uint64_t, void *> regions_;

  on_tick_t onTick_;

  tick_interval_t tickInterval_;

  volatile bool running_;

  std::mutex tickIntervalMutex_;
};

}  // namespace network

#endif
