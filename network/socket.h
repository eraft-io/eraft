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

#include <arpa/inet.h>
#include <string.h>

#include <atomic>
#include <functional>
#include <memory>
#include <string>

#define INVALID_SOCKET (int)(~0)
#define SOCKET_ERROR (-1)
#define INVALID_PORT (-1)

// os network socket address
struct SocketAddr {
  SocketAddr() { Clear(); }

  SocketAddr(const SocketAddr& other) {
    memcpy(&addr_, &other.addr_, sizeof(addr_));
  }

  SocketAddr& operator=(const SocketAddr& other) {
    if (this != &other) memcpy(&addr_, &other.addr_, sizeof(addr_));

    return *this;
  }

  SocketAddr(const sockaddr_in& addr) { Init(addr); }

  SocketAddr(const std::string& ipport) {
    std::string::size_type p = ipport.find_first_of(':');
    std::string ip = ipport.substr(0, p);
    std::string port = ipport.substr(p + 1);

    Init(ip.c_str(), static_cast<uint16_t>(std::stoi(port)));
  }

  SocketAddr(uint32_t netip, uint16_t netport) { Init(netip, netport); }

  SocketAddr(const char* ip, uint16_t hostport) { Init(ip, hostport); }

  void Init(const sockaddr_in& addr) { memcpy(&addr_, &addr, sizeof(addr)); }

  void Init(uint32_t netip, uint16_t netport) {
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = netip;
    addr_.sin_port = netport;
  }

  void Init(const char* ip, uint16_t hostport) {
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = ::inet_addr(ip);
    addr_.sin_port = htons(hostport);
  }

  const sockaddr_in& GetAddr() const { return addr_; }

  const char* GetIP() const { return ::inet_ntoa(addr_.sin_addr); }

  const char* GetIP(char* buf, socklen_t size) const {
    return ::inet_ntop(AF_INET, (const char*)&addr_.sin_addr, buf, size);
  }

  unsigned short GetPort() const { return ntohs(addr_.sin_port); }

  std::string ToString() const {
    char tmp[32];
    const char* res =
        inet_ntop(AF_INET, &addr_.sin_addr, tmp, (socklen_t)(sizeof tmp));
    return std::string(res) + ":" + std::to_string(ntohs(addr_.sin_port));
  }

  bool Empty() const { return 0 == addr_.sin_family; }
  void Clear() { memset(&addr_, 0, sizeof(addr_)); }

  inline friend bool operator==(const SocketAddr& a, const SocketAddr& b) {
    return a.addr_.sin_family == b.addr_.sin_family &&
           a.addr_.sin_addr.s_addr == b.addr_.sin_addr.s_addr &&
           a.addr_.sin_port == b.addr_.sin_port;
  }

  inline friend bool operator!=(const SocketAddr& a, const SocketAddr& b) {
    return !(a == b);
  }

  sockaddr_in addr_;
};

namespace std {
template <>
struct hash<SocketAddr> {
  typedef SocketAddr argument_type;
  typedef std::size_t result_type;
  result_type operator()(const argument_type& s) const noexcept {
    result_type h1 = std::hash<short>{}(s.addr_.sin_family);
    result_type h2 = std::hash<unsigned short>{}(s.addr_.sin_port);
    result_type h3 = std::hash<unsigned int>{}(s.addr_.sin_addr.s_addr);
    result_type tmp = h1 ^ (h2 << 1);
    return h3 ^ (tmp << 1);
  }
};
}  // namespace std

namespace Internal {
class SendThread;
}

// Abstraction for a socket
class Socket : public std::enable_shared_from_this<Socket> {
  friend class Internal::SendThread;

 public:
  virtual ~Socket();

  Socket(const Socket&) = delete;
  void operator=(const Socket&) = delete;

  enum SocketType {
    SocketType_Invalid = -1,
    SocketType_Listen,
    SocketType_Client,
    SocketType_Stream,
  };

  virtual SocketType GetSocketType() const { return SocketType_Invalid; }
  bool Invalid() const { return invalid_; }

  int GetSocket() const { return localSock_; }
  std::size_t GetID() const { return id_; }

  virtual bool OnReadable() { return false; }
  virtual bool OnWritable() { return false; }
  virtual bool OnError();
  virtual void OnConnect() {}
  virtual void OnDisconnect() {}

  static int CreateTCPSocket();
  static void CloseSocket(int& sock);
  static void SetNonBlock(int sock, bool nonBlock = true);
  static void SetNodelay(int sock);
  static void SetSndBuf(int sock, socklen_t size = 128 * 1024);
  static void SetRcvBuf(int sock, socklen_t size = 128 * 1024);
  static void SetReuseAddr(int sock);
  static bool GetLocalAddr(int sock, SocketAddr&);
  static bool GetPeerAddr(int sock, SocketAddr&);
  static void GetMyAddrInfo(unsigned int* addrs, int num);

 protected:
  Socket();

  int localSock_;
  bool epollOut_;

 private:
  std::atomic<bool> invalid_;
  std::size_t id_;
  static std::atomic<std::size_t> sid_;
};
