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

#ifndef ERAFT_KV_TICKER_H_
#define ERAFT_KV_TICKER_H_

#include <kv/router.h>

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

namespace kvserver {

class Ticker {
 public:
  typedef std::chrono::duration<int64_t, std::nano> tick_interval_t;
  typedef std::function<void()> on_tick_t;

  Ticker(std::function<void()> onTick, std::shared_ptr<Router> router,
         std::chrono::duration<int64_t, std::nano> tickInterval);
  ~Ticker();

  static void Run();

  void Start();

  void Stop();

  void SetDuration(std::chrono::duration<int64_t, std::nano> tickInterval);

  void TimerLoop();

  static Ticker* GetInstance(
      std::function<void()> onTick, std::shared_ptr<Router> router,
      std::chrono::duration<int64_t, std::nano> tickInterval) {
    if (instance_ == nullptr) {
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

}  // namespace kvserver

#endif