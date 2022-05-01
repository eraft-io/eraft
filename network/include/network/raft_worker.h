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

#ifndef ERAFT_NETWORK_RAFT_WORKER_H_
#define ERAFT_NETWORK_RAFT_WORKER_H_

#include <network/concurrency_msg_queue.h>
#include <network/raft_router.h>
#include <network/raft_store.h>

namespace network {

struct GlobalContext;

struct Router;

struct RaftPeerState;

class RaftWorker {
 public:
  RaftWorker(std::shared_ptr<GlobalContext> ctx,
             std::shared_ptr<Router> router);
  ~RaftWorker();

  static void Run(moodycamel::ConcurrentQueue<Msg> &qu);

  static void BootThread();

  static std::shared_ptr<RaftPeerState> GetPeerState(
      std::map<uint64_t, std::shared_ptr<RaftPeerState> > peerStateMap,
      uint64_t regionId);

 private:
  static std::shared_ptr<Router> router_;
  static std::shared_ptr<GlobalContext> ctx_;
  bool loop_op_;
};

}  // namespace network

#endif
