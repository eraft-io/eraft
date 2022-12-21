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

#include <sys/socket.h>
#include <sys/types.h>

#include "async_buffer.h"
#include "socket.h"

using PacketLength = int32_t;

class StreamSocket : public Socket {
  friend class SendThread;

 public:
  StreamSocket();
  ~StreamSocket();

  bool Init(int localfd, const SocketAddr &peer);
  SocketType GetSocketType() const { return SocketType_Stream; }

 public:
  int Recv();

 public:
  bool SendPacket(const void *, std::size_t);
  bool SendPacket(Buffer &bf);
  bool SendPacket(AttachedBuffer &abf);
  bool SendPacket(UnboundedBuffer &ubf);

  bool OnReadable();
  bool OnWriteable();
  bool OnError();

  bool DoMsgParse();

  void SetOnDisconnect(
      const std::function<void()> &cb = std::function<void()>()) {
    onDisconnect_ = cb;
  }

  bool Send();

  const SocketAddr &GetPeerAddr() const { return peerAddr_; }

 protected:
  SocketAddr peerAddr_;

 private:
  std::function<void()> onDisconnect_;

  // send buf to peer
  int _Send(const BufferSequence &bf);
  virtual PacketLength _HandlePacket(const char *msg, std::size_t len) = 0;

  enum {
    TIMEOUTSOCKET = 0,
    ERRORSOCKET = -1,
    EOFSOCKET = -2,
  };

  Buffer recvBuf_;

  AsyncBuffer sendBuf_;
};
