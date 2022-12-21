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

class AuthPacket {
 public:
  ~AuthPacket();
  AuthPacket(const AuthPacket&) = delete;
  AuthPacket(AuthPacket&&) = delete;
  AuthPacket& operator=(const AuthPacket&) = delete;
  AuthPacket& operator=(AuthPacket&&) = delete;
  AuthPacket();
  /**
   * 编码 auth 包
   * TODO 暂时没有编码的需求
   */
  std::vector<uint8_t> Pack(uint32_t capabilityFlags, uint8_t charset,
                            std::string username, std::string password,
                            std::vector<uint8_t> salt, std::string database);
  /**
   * 解码 auth 包
   */
  bool UnPack(std::vector<uint8_t>& packet);

  std::string GetPluginName();

  std::string GetDatabaseName();

  std::string GetUserName();

 private:
  struct Impl;

  std::unique_ptr<Impl> impl_;
};

}  // namespace Protocol