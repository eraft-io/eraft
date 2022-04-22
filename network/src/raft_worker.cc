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

#include <network/raft_worker.h>

namespace network {

RaftWorker::RaftWorker(std::shared_ptr<GlobalContext> ctx,
                       std::shared_ptr<Router> pm) {}
RaftWorker::~RaftWorker() {}

void RaftWorker::Run(moodycamel::ConcurrentQueue<Msg> &qu) {
  // 1.loop true op

  // 2.pop msg from dequeue

  // 3.handle msg

  // 4.raft ready
}

void RaftWorker::BootThread() {}

std::shared_ptr<RaftPeerState> RaftWorker::GetPeerState(
    std::map<uint64_t, std::shared_ptr<RaftPeerState> > peerStateMap,
    uint64_t regionId) {}

}  // namespace network
