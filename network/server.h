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

#include <functional>
#include <set>

#include "task_manager.h"

struct SocketAddr;

class Server {
 protected:
  virtual bool _RunLogic();
  virtual bool _Recycle() { return true; }
  virtual bool _Init() = 0;

  Server();

  Server(const Server &) = delete;
  void operator=(const Server &) = delete;

 public:
  virtual ~Server();

  bool TCPBind(const SocketAddr &listenAddr, int tag);
  void TCPReconnect(const SocketAddr &peer, int tag);

  static Server *Instance() { return sinstance_; }

  bool IsTerminate() const { return bTerminate_; }
  void Terminate() { bTerminate_ = true; }

  void MainLoop(bool daemon = false);
  void NewConnection(int sock, int tag,
                     const std::function<void()> &cb = std::function<void()>());

  void TCPConnect(const SocketAddr &peer, int tag);
  void TCPConnect(const SocketAddr &peer, const std::function<void()> &cb,
                  int tag);

  size_t TCPSize() const { return tasks_.TCPSize(); }

  virtual void ReloadConfig() {}

  static void IntHandler(int sig);
  static void HupHandler(int sig);

  std::shared_ptr<StreamSocket> FindTCP(unsigned int id) const {
    return tasks_.FindTCP(id);
  }

  static void AtForkHandler();
  static void DelListenSock(int sock);

 private:
  virtual std::shared_ptr<StreamSocket> _OnNewConnection(int tcpsock, int tag);
  void _TCPConnect(const SocketAddr &peer,
                   const std::function<void()> *cb = nullptr, int tag = -1);

  std::atomic<bool> bTerminate_;
  Internal::TaskManager tasks_;
  bool reloadCfg_;
  static Server *sinstance_;
  static std::set<int> slistenSocks_;
};
