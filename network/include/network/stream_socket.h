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

#ifndef ERAFT_STREAM_SOCKET_H_
#define ERAFT_STREAM_SOCKET_H_

#include <network/async_buffer.h>
#include <network/socket.h>
#include <sys/socket.h>
#include <sys/types.h>

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

#endif