// Copyright 2022 The uhp-sql Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include <unistd.h>

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

#include "poller.h"
#include "thread_pool.h"

inline long GetCpuNum() { return sysconf(_SC_NPROCESSORS_ONLN); }

class Socket;
typedef std::shared_ptr<Socket> PSOCKET;

namespace Internal {

class NetThread {
 public:
  NetThread();
  virtual ~NetThread();

  bool IsAlive() const { return runnning_; }
  void Stop() { runnning_ = false; }

  void AddSocket(PSOCKET, uint32_t event);
  void ModSocket(PSOCKET, uint32_t event);
  void RemoveSocket(PSOCKET, uint32_t event);

 protected:
  std::unique_ptr<Poller> poller_;
  std::vector<FiredEvent> firedEvents_;
  std::deque<PSOCKET> tasks_;
  void _TryAddNewTasks();

 private:
  std::atomic<bool> runnning_;

  std::mutex mutex_;
  typedef std::vector<std::pair<std::shared_ptr<Socket>, uint32_t>> NewTasks;
  NewTasks newTasks_;
  std::atomic<int> newCnt_;
  void _AddSocket(PSOCKET, uint32_t event);
};

class RecvThread : public NetThread {
 public:
  void Run();
};

class SendThread : public NetThread {
 public:
  void Run();
};

class NetThreadPool {
  std::shared_ptr<RecvThread> recvThread_;
  std::shared_ptr<SendThread> sendThread_;

 public:
  NetThreadPool() = default;

  NetThreadPool(const NetThreadPool &) = delete;
  void operator=(const NetThreadPool &) = delete;

  bool AddSocket(PSOCKET, uint32_t event);
  bool StartAllThreads();
  bool StopAllThreads();

  void EnableRead(const std::shared_ptr<Socket> &sock);
  void EnableWrite(const std::shared_ptr<Socket> &sock);
  void DisableRead(const std::shared_ptr<Socket> &sock);
  void DisableWrite(const std::shared_ptr<Socket> &sock);

  static NetThreadPool &Instance() {
    static NetThreadPool pool;
    return pool;
  }
};

}  // namespace Internal
