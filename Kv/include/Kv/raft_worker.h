// MIT License

// Copyright (c) 2021 Colin

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ERAFT_KV_RAFT_WORKER_H_
#define ERAFT_KV_RAFT_WORKER_H_

#include <deque>
#include <memory>
#include <map>

#include <Kv/raft_store.h>
#include <Kv/msg.h>
#include <Kv/router.h>
#include <Kv/concurrency_queue.h>

namespace kvserver
{

struct GlobalContext;
struct Router;
struct PeerState_;

class RaftWorker
{

public:

  RaftWorker(std::shared_ptr<GlobalContext> ctx, std::shared_ptr<Router> pm);
  ~RaftWorker();

  static void Run(Queue<Msg>& qu);

  static void BootThread();

  static std::shared_ptr<PeerState_> GetPeerState(std::map<uint64_t, std::shared_ptr<PeerState_> > peersStateMap, uint64_t regionID);

private:

  static std::shared_ptr<Router> pr_;
  
  static std::shared_ptr<GlobalContext> ctx_;

};


} // namespace kvserver


#endif