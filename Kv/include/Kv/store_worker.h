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

#ifndef ERAFT_KV_STORE_WORKER_H_
#define ERAFT_KV_STORE_WORKER_H_

#include <Kv/concurrency_queue.h>
#include <Kv/msg.h>
#include <Kv/raft_store.h>
#include <eraftio/raft_serverpb.pb.h>
#include <stdint.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

namespace kvserver {

using StoreTick = int;

class StoreWorker {
 public:
  StoreWorker(std::shared_ptr<GlobalContext> ctx,
              std::shared_ptr<StoreState> state);
  ~StoreWorker();

  static bool IsAlive() { return running_; }

  static void Stop() { running_ = false; }

  static void Run(Queue<Msg>& qu);

  static void BootThread();

  static void OnTick(StoreTick* tick);

  static void HandleMsg(Msg msg);

  static bool OnRaftMessage(raft_serverpb::RaftMessage* msg);

 protected:
 private:
  static uint64_t id_;

  static bool running_;

  // StoreState
  static std::shared_ptr<StoreState> storeState_;

  static std::shared_ptr<GlobalContext> ctx_;
};

}  // namespace kvserver

#endif
