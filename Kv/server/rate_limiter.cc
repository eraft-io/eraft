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

#include <Kv/rate_limiter.h>
#include <sys/time.h>
#include <unistd.h>

namespace kvserver {

RateLimiter* RateLimiter::instance_ = nullptr;

RateLimiter::RateLimiter(int64_t capacity)
    : capacity_(capacity),
      failed_count_(0),
      quantum_(0),
      available_tokens_(0),
      fill_interval_(US_PER_SECOND / capacity) {
  latest_tick_ = now();
}

RateLimiter::~RateLimiter() {}

int64_t RateLimiter::now() {
  struct timeval tv;
  ::gettimeofday(&tv, 0);
  int64_t seconds = tv.tv_sec;
  return seconds * US_PER_SECOND + tv.tv_usec;
}

void RateLimiter::SupplyTokens() {
  int64_t cur = now();
  if (cur - latest_tick_ < fill_interval_) {
    return;
  }
  {
    std::unique_lock<std::mutex> mlock(mutex_);
    quantum_ = (cur - latest_tick_) / fill_interval_;
    if (quantum_ <= 0) {
      return;
    }
    latest_tick_ += (quantum_ * fill_interval_);
    available_tokens_ += quantum_;
    if (available_tokens_ >= capacity_) {
      available_tokens_ = capacity_;
    }
    mlock.unlock();
  }
  return;
}

bool RateLimiter::TryGetToken() {
  SupplyTokens();
  {
    std::unique_lock<std::mutex> mlock(mutex_);
    if (available_tokens_ > 0) {
      available_tokens_--;
      return true;
    }
    mlock.unlock();
  }
  failed_count_++;
  if (failed_count_ > 1000000) {
    failed_count_ = 0;
  }
  return false;
}

uint64_t RateLimiter::GetFailedCount() { return failed_count_; }

void RateLimiter::TestPass() {}

void RateLimiter::SetCapacity(int64_t capacity) {
  capacity_ = capacity;
  fill_interval_ = US_PER_SECOND / capacity_;
}

}  // namespace kvserver
