/**
 * @file listen_socket.cc
 * @author https://github.com/loveyacper/Qedis
 * @brief
 * @version 0.1
 * @date 2023-06-17
 *
 * @copyright Copyright (c) 2023
 *
 */

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

ListenSocket::~ListenSocket() {
  Server::Instance()->DelListenSock(localSock_);
}

bool ListenSocket::Bind(const SocketAddr &addr) {
  if (addr.Empty())
    return false;

  if (localSock_ != INVALID_SOCKET)
    return false;

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

bool ListenSocket::OnWriteable() {
  return false;
}

bool ListenSocket::OnError() {
  if (Socket::OnError()) {
    Server::Instance()->DelListenSock(localSock_);
    return true;
  }

  return false;
}

}  // namespace Internal