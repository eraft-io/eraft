/**
 * @file listen_socket.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-06-17
 *
 * @copyright Copyright (c) 2023
 *
 */


#pragma once

#include "socket.h"

namespace Internal {

class ListenSocket : public Socket {
  static const int LISTENQ;

 public:
  explicit ListenSocket(int tag);
  ~ListenSocket();

  SocketType GetSocketType() const {
    return SocketType_Listen;
  }

  bool Bind(const SocketAddr &addr);
  bool OnReadable();
  bool OnWriteable();
  bool OnError();

 private:
  int            _Accept();
  sockaddr_in    addrClient_;
  unsigned short localPort_;
  const int      tag_;
};

};  // namespace Internal
