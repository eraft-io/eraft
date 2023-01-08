#include "sync_client.h"
#include <sys/time.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

//
// dealwith sync packet
//
PacketLength SyncClient::_HandlePacket(const char *start, std::size_t bytes) {
  const char *const end = start + bytes;
  const char *ptr = start;
  return static_cast<PacketLength>(bytes);
}

SyncClient::SyncClient() { _Reset(); }

void SyncClient::_Reset() {
  parser_.Reset();
  reply_.Clear();
}

SyncClient::~SyncClient() {}

void SyncClient::OnConnect() {
  std::cout << "new client comming!" << std::endl;
  _Reset();
}
