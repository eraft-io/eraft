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

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

class StreamSocket;

namespace Internal {

class TaskManager {
  typedef std::shared_ptr<StreamSocket> PTCPSOCKET;
  typedef std::vector<PTCPSOCKET> NEWTASKS_T;

 public:
  TaskManager() : newCnt_(0) {}
  ~TaskManager();

  bool AddTask(PTCPSOCKET);

  bool Empty() const { return tcpSockets_.empty(); }
  void Clear() { tcpSockets_.clear(); }
  PTCPSOCKET FindTCP(unsigned int id) const;

  size_t TCPSize() const { return tcpSockets_.size(); }

  bool DoMsgParse();

 private:
  bool _AddTask(PTCPSOCKET task);
  void _RemoveTask(std::map<int, PTCPSOCKET>::iterator&);
  std::map<int, PTCPSOCKET> tcpSockets_;

  std::mutex lock_;
  NEWTASKS_T newTasks_;
  std::atomic<int> newCnt_;
};

}  // namespace Internal
