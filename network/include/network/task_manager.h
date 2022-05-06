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

#ifndef ERAFT_TASK_MANAGER_H_
#define ERAFT_TASK_MANAGER_H_

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

#endif