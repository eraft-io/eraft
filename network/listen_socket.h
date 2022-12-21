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

#include "socket.h"

namespace Internal {

class ListenSocket : public Socket {
  static const int LISTENQ;

 public:
  explicit ListenSocket(int tag);
  ~ListenSocket();

  SocketType GetSocketType() const { return SocketType_Listen; }

  bool Bind(const SocketAddr &addr);
  bool OnReadable();
  bool OnWriteable();
  bool OnError();

 private:
  int _Accept();
  sockaddr_in addrClient_;
  unsigned short localPort_;
  const int tag_;
};

};  // namespace Internal
