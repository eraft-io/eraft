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

#ifndef ERAFT_NET_THREAD_POOL_H_
#define ERAFT_NET_THREAD_POOL_H_

#include <network/poller.h>
#include <network/thread_pool.h>
#include <unistd.h>

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

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

#endif
