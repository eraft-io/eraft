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
// !!! WARRING !!!: support mysql protocol version <= mysql-5.1.70
//

#include "greeting.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <vector>

#include "common.h"
// #include <SystemPort/CryptoRandom.hpp>

namespace Protocol {

struct GreetingPacket::Impl {
  /**
   *  协议版本号
   *  protocol version in greeting packet.
   */
  uint8_t protocolVersion;

  /**
   *  字符编码
   *  charset in greeting packet.
   */
  uint8_t charset;

  /**
   * 服务器状态
   * status in greeting packet.
   */
  uint16_t status;

  /**
   * 服务器权能标识
   * Server capability flag
   */
  uint32_t capability;

  /**
   * 服务器线程ID
   * Server connetion id in greeting packet.
   */
  uint32_t connectionID;

  /**
   * 服务器版本号
   *
   * Server Version in greeting packet.
   */
  std::string serverVersion;

  /**
   * 用户插件名
   *
   * auth plugin name in greeting packet.
   */
  std::string authPluginName;

  /**
   *  挑战随机数 Part One
   *
   *  Challenge random number in greeting packet.
   */
  std::vector<uint8_t> saltPart1;

  /**
   *  挑战随机数 Part Two
   *
   *  Challenge random number in greeting packet.
   */
  std::vector<uint8_t> saltPart2;

  /**
   *  PluginData 长度
   *  Length of auth plugin data, Always 21
   */
  uint8_t authPluginDataLength;
};

GreetingPacket::GreetingPacket() : impl_(new Impl()) {}

GreetingPacket::~GreetingPacket() {
  // auto
}

GreetingPacket::GreetingPacket(uint32_t connectionID, std::string serverVersion)
    : impl_(new Impl()) {
  impl_->protocolVersion = 10;
  impl_->serverVersion = serverVersion;
  impl_->connectionID = connectionID;
  impl_->capability = static_cast<uint32_t> DEFAULT_SERVER_CAPABILITY;
  impl_->charset = static_cast<uint8_t>(CHARACTER_SET_UTF8);
  impl_->status = static_cast<uint16_t>(SERVER_STATUS_AUTOCOMMIT);
  impl_->saltPart1.resize(8);

  int rng = open("/dev/urandom", O_RDONLY);
  uint8_t buffer;
  for (size_t i = 0; i < 8; i++) {
    (void)read(rng, &buffer, 1);
    impl_->saltPart1[i] = buffer % 123;
  }
  impl_->saltPart2.resize(12);
  for (size_t i = 0; i < 12; i++) {
    (void)read(rng, &buffer, 1);
    impl_->saltPart2[i] = buffer % 123;
  }
}

/**
 * Pack greeting obj to bytes seq
 */
std::vector<uint8_t> GreetingPacket::Pack() {
  std::vector<uint8_t> packetOutput;
  // packetOutput.resize(256);
  uint16_t capLower = static_cast<uint16_t>(impl_->capability);
  uint16_t capUpper = static_cast<uint16_t>(impl_->capability >> 16);
  // protocolVersion
  packetOutput.push_back(impl_->protocolVersion);

  // string[NUL] server version
  packetOutput.insert(packetOutput.end(), impl_->serverVersion.begin(),
                      impl_->serverVersion.end());

  // zero
  packetOutput.push_back(0x00);

  // connection id
  packetOutput.push_back(static_cast<uint8_t>(impl_->connectionID));
  packetOutput.push_back(static_cast<uint8_t>(impl_->connectionID >> 8));
  packetOutput.push_back(static_cast<uint8_t>(impl_->connectionID >> 16));
  packetOutput.push_back(static_cast<uint8_t>(impl_->connectionID >> 24));

  // auth plugin-data-part 1
  packetOutput.insert(packetOutput.end(), impl_->saltPart1.begin(),
                      impl_->saltPart1.end());

  // filter
  packetOutput.push_back(0x00);

  // capLower
  packetOutput.push_back(static_cast<uint8_t>(capLower));
  packetOutput.push_back(static_cast<uint8_t>(capLower >> 8));

  // character set
  packetOutput.push_back(impl_->charset);

  // status flags
  packetOutput.push_back(static_cast<uint8_t>(impl_->status));
  packetOutput.push_back(static_cast<uint8_t>(impl_->status >> 8));

  // capUpper
  packetOutput.push_back(static_cast<uint8_t>(capUpper));
  packetOutput.push_back(static_cast<uint8_t>(capUpper >> 8));

  // Length of auth plugin data, Always 21
  packetOutput.push_back(21);

  // string[10] reserved
  for (size_t i = 0; i < 10; i++) {
    packetOutput.push_back(0x00);
  }

  // auth plugin-data-part 2
  packetOutput.insert(packetOutput.end(), impl_->saltPart2.begin(),
                      impl_->saltPart2.end());

  // 0
  packetOutput.push_back(0x00);

  std::string pluginName = "mysql_native_password";
  packetOutput.insert(packetOutput.end(), pluginName.begin(), pluginName.end());
  packetOutput.push_back(0x00);
  return packetOutput;
}

bool GreetingPacket::UnPack(std::vector<uint8_t>& packet) {
  uint16_t capLower, capUpper;
  std::vector<uint8_t>::iterator seek = packet.begin();
  // protocolVersion
  impl_->protocolVersion = packet.front();
  seek++;
  // serverVersion
  std::vector<uint8_t>::iterator serverVersionEnd =
      std::find(seek, packet.end(), 0x00);
  if (serverVersionEnd == packet.end()) {
    return false;
  }
  impl_->serverVersion = std::string(seek, serverVersionEnd);
  seek = serverVersionEnd;
  // 0
  seek++;
  // connection id
  impl_->connectionID = static_cast<uint32_t>(*seek) |
                        static_cast<uint32_t>(*(seek + 1) << 8) |
                        static_cast<uint32_t>(*(seek + 2) << 16) |
                        static_cast<uint32_t>(*(seek + 3) << 24);
  seek += 4;
  // auth plugin-data-part 1
  impl_->saltPart1 = std::vector<uint8_t>(seek, seek + 8);
  seek += 8;
  // 0 filter
  seek++;
  // capLower
  capLower =
      static_cast<uint16_t>(*seek) | static_cast<uint16_t>(*(seek + 1) << 8);
  seek += 2;
  // character set
  impl_->charset = *seek;
  seek++;
  // status flags
  impl_->status =
      static_cast<uint16_t>(*seek) | static_cast<uint16_t>(*(seek + 1) << 8);
  seek += 2;
  // capUpper
  capUpper =
      static_cast<uint16_t>(*seek) | static_cast<uint16_t>(*(seek + 1) << 8);
  seek += 2;
  impl_->capability =
      static_cast<uint32_t>(capUpper) << 16 | static_cast<uint32_t>(capLower);
  impl_->authPluginDataLength = *seek;
  seek++;
  // string[10] reserved
  seek += 10;
  // auth plugin-data-part 2
  impl_->saltPart2 = std::vector<uint8_t>(seek, seek + 12);
  seek += 12;
  // 0 filter
  seek++;
  std::vector<uint8_t>::iterator authPluginNameEnd =
      std::find(seek, packet.end(), 0x00);
  if (authPluginNameEnd == packet.end()) {
    return false;
  }
  impl_->authPluginName = std::string(seek, authPluginNameEnd);
  return true;
}

std::string GreetingPacket::GetServerVersion() { return impl_->serverVersion; }

uint32_t GreetingPacket::GetConnectionID() { return impl_->connectionID; }

std::string GreetingPacket::GetAuthPluginName() {
  return impl_->authPluginName;
}

uint16_t GreetingPacket::GetServerStatus() { return impl_->status; }

}  // namespace Protocol
