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

#ifndef ERAFT_KV_RATELIMITER_H_
#define ERAFT_KV_RATELIMITER_H_

#include <mutex>

namespace kvserver {

class RateLimiter {
 public:
  RateLimiter(int64_t capacity);

  ~RateLimiter();

  int64_t now();

  void SupplyTokens();

  bool TryGetToken();

  void TestPass();

  void SetCapacity(int64_t capacity);

  static RateLimiter* GetInstance() {
    if (instance_ == nullptr) {
      instance_ = new RateLimiter(50);
    }
    return instance_;
  }

 protected:
  static RateLimiter* instance_;

 private:
  // capacity holds the overall capacity of the bucket.
  int64_t capacity_;

  // quantum holds how many tokens are added on each tick.
  int64_t quantum_;

  // availableTokens holds the number of available
  // tokens as of the associated latestTick.
  // It will be negative when there are consumers
  // waiting for tokens.
  int64_t available_tokens_;

  // latestTick holds the latest tick for which
  // we know the number of tokens in the bucket.
  int64_t latest_tick_;

  // fillInterval holds the interval between each tick.
  int64_t fill_interval_;

  // auto time_;

  std::mutex mutex_;

  static const uint64_t US_PER_SECOND = 1000000;
};

}  // namespace kvserver

#endif  // rate_limit