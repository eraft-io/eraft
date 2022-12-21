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

#include "task_manager.h"

#include <cassert>

#include "stream_socket.h"

namespace Internal {

TaskManager::~TaskManager() {
  assert(Empty() && "Why you do not clear container before exit?");
}

bool TaskManager::AddTask(PTCPSOCKET task) {
  std::lock_guard<std::mutex> guard(lock_);
  newTasks_.push_back(task);
  ++newCnt_;

  return true;
}

TaskManager::PTCPSOCKET TaskManager::FindTCP(unsigned int id) const {
  if (id > 0) {
    auto it = tcpSockets_.find(id);
    if (it != tcpSockets_.end()) return it->second;
  }

  return PTCPSOCKET();
}

bool TaskManager::_AddTask(PTCPSOCKET task) {
  bool succ = tcpSockets_.insert({task->GetID(), task}).second;
  return succ;
}

void TaskManager::_RemoveTask(std::map<int, PTCPSOCKET>::iterator &it) {
  tcpSockets_.erase(it++);
}

bool TaskManager::DoMsgParse() {
  if (newCnt_ > 0 && lock_.try_lock()) {
    NEWTASKS_T tmpNewTask;
    tmpNewTask.swap(newTasks_);
    newCnt_ = 0;
    lock_.unlock();

    for (const auto &task : tmpNewTask) {
      if (!_AddTask(task)) {
      } else {
        // new Connectin form
        task->OnConnect();
      }
    }
  }

  bool busy = false;

  for (auto it(tcpSockets_.begin()); it != tcpSockets_.end();) {
    if (!it->second || it->second->Invalid()) {
      if (it->second) {
        it->second->OnDisconnect();
      }
      _RemoveTask(it);
    } else {
      if (it->second->DoMsgParse() && !busy) busy = true;

      ++it;
    }
  }

  return busy;
}

}  // namespace Internal
