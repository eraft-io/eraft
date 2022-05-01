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

#include <network/raft_peer_msg_handler.h>
#include <network/raft_worker.h>

namespace network {

std::shared_ptr<Router> RaftWorker::router_ = nullptr;

std::shared_ptr<GlobalContext> RaftWorker::ctx_ = nullptr;

RaftWorker::RaftWorker(std::shared_ptr<GlobalContext> ctx,
                       std::shared_ptr<Router> router) {
  RaftWorker::ctx_ = ctx;
  RaftWorker::router_ = router;
  loop_op_ = true;
}

RaftWorker::~RaftWorker() {}

void RaftWorker::Run(moodycamel::ConcurrentQueue<Msg> &qu) {
  SPDLOG_INFO("raft worker start running!");
  std::map<uint64_t, std::shared_ptr<RaftPeerState> > peerStMap;
  // 1.loop true op
  while (true) {
    Msg msg;
    // 2.pop msg from dequeue
    if (!qu.try_dequeue(msg)) {
      continue;
    }
    SPDLOG_INFO("pop new messsage with type: " + msg.MsgToString());

    std::shared_ptr<RaftPeerState> peerState =
        RaftWorker::GetPeerState(peerStMap, msg.regionId_);
    if (peerState == nullptr) {
      continue;
    }

    // 3.handle msg
    RaftPeerMsgHandler pmHandler(peerState->peer_, RaftWorker::ctx_);
    pmHandler.HandleMsg(msg);

    // 4.raft ready
    RaftPeerMsgHandler pmHandlerRaftRd(peerState->peer_, RaftWorker::ctx_);
    pmHandlerRaftRd.HandleRaftReady();
  }
}

void RaftWorker::BootThread() {
  std::thread th(
      std::bind(&Run, std::ref(QueueContext::GetInstance()->get_peerSender())));
  th.detach();
}

std::shared_ptr<RaftPeerState> RaftWorker::GetPeerState(
    std::map<uint64_t, std::shared_ptr<RaftPeerState> > peersStateMap,
    uint64_t regionId) {
  if (peersStateMap.find(regionId) == peersStateMap.end()) {
    auto peer = RaftWorker::router_->Get(regionId);
    if (peer == nullptr) {
      return nullptr;
    }
    peersStateMap[regionId] = peer;
  }
  return peersStateMap[regionId];
}

}  // namespace network
