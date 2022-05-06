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

#ifndef ERAFT_SERVER_H_
#define ERAFT_SERVER_H_

#include <network/task_manager.h>

#include <functional>
#include <set>

struct SocketAddr;

class Server {
 protected:
  virtual bool _RunLogic();
  virtual bool _Recycle() {}
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

#endif