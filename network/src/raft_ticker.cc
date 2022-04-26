// MIT License

// Copyright (c) 2022 eraft dev group

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

#include <network/raft_ticker.h>

namespace network {

Ticker *Ticker::instance_ = nullptr;
std::map<uint64_t, void *> Ticker::regions_ = {};

Ticker::Ticker(on_tick_t onTick, tick_interval_t tickInterval)
    : onTick_(onTick), tickInterval_(tickInterval), running_(false) {}

Ticker::~Ticker() { delete instance_; }

void Ticker::Start() {
  if (running_) {
    return;
  }
  running_ = true;
  std::thread runT(&Ticker::TimerLoop, this);
  runT.detach();
}

void Ticker::Stop() { running_ = false; }

void Ticker::Run() {
  for (auto r : regions_) {
    auto msg = NewPeerMsg(MsgType::MsgTypeTick, r.first, nullptr);
    router_->Send(r.first, msg);
  }
  uint64_t regionId = 0;
  QueueContext::GetInstance()->get_regionIdCh().try_dequeue(regionId);
  regions_.insert(std::pair<uint64_t, void *>(regionId, nullptr));
}

void Ticker::SetDuration(tick_interval_t tickInterval) {
  tickIntervalMutex_.lock();
  tickInterval_ = tickInterval;
  tickIntervalMutex_.unlock();
}

void Ticker::TimerLoop() {
  while (running_) {
    std::thread runT(onTick_);
    runT.detach();
    tickIntervalMutex_.lock();
    std::chrono::duration<int64_t, std::nano> tickInterval = tickInterval_;
    tickIntervalMutex_.unlock();
    std::this_thread::sleep_for(tickInterval);
  }
}

}  // namespace network
