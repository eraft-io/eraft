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

#include "listen_socket.h"

#include <errno.h>
#include <sys/socket.h>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "net_thread_pool.h"
#include "server.h"

namespace Internal {

const int ListenSocket::LISTENQ = 1024;

ListenSocket::ListenSocket(int tag) : localPort_(INVALID_PORT), tag_(tag) {}

ListenSocket::~ListenSocket() { Server::Instance()->DelListenSock(localSock_); }

bool ListenSocket::Bind(const SocketAddr &addr) {
  if (addr.Empty()) return false;

  if (localSock_ != INVALID_SOCKET) return false;

  localPort_ = addr.GetPort();
  localSock_ = CreateTCPSocket();
  SetNonBlock(localSock_);
  SetNodelay(localSock_);
  SetReuseAddr(localSock_);
  SetRcvBuf(localSock_);
  SetSndBuf(localSock_);

  struct sockaddr_in serv = addr.GetAddr();

  int ret = ::bind(localSock_, (struct sockaddr *)&serv, sizeof serv);
  if (SOCKET_ERROR == ret) {
    CloseSocket(localSock_);
    return false;
  }
  ret = ::listen(localSock_, ListenSocket::LISTENQ);
  if (SOCKET_ERROR == ret) {
    CloseSocket(localSock_);
    return false;
  }

  if (!NetThreadPool::Instance().AddSocket(shared_from_this(), EventTypeRead))
    return false;

  return true;
}

int ListenSocket::_Accept() {
  socklen_t addrLen = sizeof addrClient_;
  return ::accept(localSock_, (struct sockaddr *)&addrClient_, &addrLen);
}

bool ListenSocket::OnReadable() {
  // wait client connection comming
  while (true) {
    int connfd = _Accept();
    if (connfd >= 0) {
      Server::Instance()->NewConnection(connfd, tag_);
    } else {
      bool result = false;
      switch (errno) {
        case EWOULDBLOCK:
        case ECONNABORTED:
        case EINTR:
          result = true;
          break;
        case EMFILE:
        case ENFILE:
          // Log not enough file discriptor available
          result = true;
          break;
        case ENOBUFS:
          // not enough memory
          result = true;

        default:
          break;
      }

      return result;
    }
  }

  return true;
}

bool ListenSocket::OnWriteable() { return false; }

bool ListenSocket::OnError() {
  if (Socket::OnError()) {
    Server::Instance()->DelListenSock(localSock_);
    return true;
  }

  return false;
}

}  // namespace Internal