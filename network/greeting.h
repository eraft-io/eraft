
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

#pragma once

#include <unistd.h>

#include <iostream>
#include <memory>
#include <vector>

namespace Protocol {

class GreetingPacket {
 public:
  ~GreetingPacket();
  GreetingPacket(const GreetingPacket&) = delete;
  GreetingPacket(GreetingPacket&&) = delete;
  GreetingPacket& operator=(const GreetingPacket&) = delete;
  GreetingPacket& operator=(GreetingPacket&&) = delete;
  GreetingPacket();

  GreetingPacket(uint32_t connectionID, std::string serverVersion);
  /**
   * 编码 greeting 包
   */
  std::vector<uint8_t> Pack();

  std::string GetServerVersion();
  uint32_t GetConnectionID();
  std::string GetAuthPluginName();
  uint16_t GetServerStatus();

  /**
   * 解码 greeting 包
   */
  bool UnPack(std::vector<uint8_t>& packet);

 private:
  struct Impl;

  std::unique_ptr<Impl> impl_;
};

}  // namespace Protocol