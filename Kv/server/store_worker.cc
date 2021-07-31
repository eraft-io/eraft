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

#include <Kv/store_worker.h>
#include <Logger/logger.h>

namespace kvserver {

uint64_t StoreWorker::id_ = 0;

bool StoreWorker::running_ = true;

// StoreState
std::shared_ptr<StoreState> StoreWorker::storeState_ = nullptr;

std::shared_ptr<GlobalContext> StoreWorker::ctx_ = nullptr;

StoreWorker::StoreWorker(std::shared_ptr<GlobalContext> ctx,
                         std::shared_ptr<StoreState> state) {
  StoreWorker::storeState_ = state;
  StoreWorker::ctx_ = ctx;
}

StoreWorker::~StoreWorker() {}

void StoreWorker::BootThread() {
  std::thread th(
      std::bind(&Run, std::ref(QueueContext::GetInstance()->storeSender_)));
  th.detach();
}

void StoreWorker::Run(Queue<Msg>& qu) {
  Logger::GetInstance()->DEBUG_NEW("store worker running!", __FILE__, __LINE__,
                                   "StoreWorker::Run");

  while (IsAlive()) {
    auto msg = qu.Pop();
    Logger::GetInstance()->DEBUG_NEW("pop new messsage from store sender",
                                     __FILE__, __LINE__, "StoreWorker::Run");
    StoreWorker::HandleMsg(msg);
  }
}

void StoreWorker::OnTick(StoreTick* tick) {}

void StoreWorker::HandleMsg(Msg msg) {
  switch (msg.type_) {
    case MsgType::MsgTypeStoreRaftMessage: {
      if (!StoreWorker::OnRaftMessage(
              static_cast<raft_serverpb::RaftMessage*>(msg.data_))) {
      }
      break;
    }
    case MsgType::MsgTypeStoreTick: {
      StoreWorker::OnTick(static_cast<StoreTick*>(msg.data_));
      break;
    }
    case MsgType::MsgTypeStoreStart: {
      StoreWorker::Start(static_cast<metapb::Store*>(msg.data_));
      break;
    }
    default:
      break;
  }
}

void StoreWorker::Start(metapb::Store* store) {}

bool StoreWorker::CheckMsg(std::shared_ptr<raft_serverpb::RaftMessage> msg) {}

bool StoreWorker::OnRaftMessage(raft_serverpb::RaftMessage* msg) {}

bool StoreWorker::MaybeCreatePeer(
    uint64_t regionID, std::shared_ptr<raft_serverpb::RaftMessage> msg) {}

void StoreWorker::StoreHeartbeatScheduler() {}

void StoreWorker::OnSchedulerStoreHeartbeatTick() {}

bool StoreWorker::HandleSnapMgrGC() {}

bool StoreWorker::ScheduleGCSnap()  //  TODO:
{}

}  // namespace kvserver
