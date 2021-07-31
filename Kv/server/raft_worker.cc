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

#include <Kv/peer_msg_handler.h>
#include <Kv/raft_worker.h>
#include <Logger/logger.h>

#include <condition_variable>
#include <iostream>
#include <thread>

namespace kvserver {

std::shared_ptr<GlobalContext> RaftWorker::ctx_ = nullptr;
std::shared_ptr<Router> RaftWorker::pr_ = nullptr;

RaftWorker::RaftWorker(std::shared_ptr<GlobalContext> ctx,
                       std::shared_ptr<Router> pm) {
  RaftWorker::ctx_ = ctx;
  RaftWorker::pr_ = pm;
}

void RaftWorker::BootThread() {
  std::thread th(
      std::bind(&Run, std::ref(QueueContext::GetInstance()->peerSender_)));
  th.detach();
}

void RaftWorker::Run(Queue<Msg>& qu) {
  Logger::GetInstance()->DEBUG_NEW("raft worker start running!", __FILE__,
                                   __LINE__, "RaftWorker::Run");

  std::map<uint64_t, std::shared_ptr<PeerState_> > peerStMap;

  while (true) {
    auto msg = qu.Pop();
    Logger::GetInstance()->DEBUG_NEW(
        "pop new messsage with type: " + msg.MsgToString(), __FILE__, __LINE__,
        "RaftWorker::Run");

    // handle m, call PeerMessageHandler
    std::shared_ptr<PeerState_> peerState =
        RaftWorker::GetPeerState(peerStMap, msg.regionId_);
    if (peerState == nullptr) {
      continue;
    }

    PeerMsgHandler pmHandler(peerState->peer_, RaftWorker::ctx_);
    pmHandler.HandleMsg(msg);

    // get peer state, and handle raft ready
    PeerMsgHandler pmHandlerRaftRd(peerState->peer_, RaftWorker::ctx_);
    pmHandlerRaftRd.HandleRaftReady();
  }
}

RaftWorker::~RaftWorker() {}

std::shared_ptr<PeerState_> RaftWorker::GetPeerState(
    std::map<uint64_t, std::shared_ptr<PeerState_> > peersStateMap,
    uint64_t regionID) {
  if (peersStateMap.find(regionID) == peersStateMap.end()) {
    auto peer = RaftWorker::pr_->Get(regionID);
    if (peer == nullptr) {
      return nullptr;
    }
    peersStateMap[regionID] = peer;
  }
  return peersStateMap[regionID];
}

}  // namespace kvserver
