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

class EofPacket {
 public:
  ~EofPacket() noexcept;
  EofPacket(const EofPacket&) = delete;
  EofPacket(EofPacket&&) noexcept = delete;
  EofPacket& operator=(const EofPacket&) = delete;
  EofPacket& operator=(EofPacket&&) noexcept = delete;
  EofPacket();

  EofPacket(uint16_t warnFlag, uint16_t statusFlag);
  /**
   * 编码 Eof 包
   */
  std::vector<uint8_t> Pack();

  /**
   * 解码 Eof 包
   */
  bool UnPack(std::vector<uint8_t>& packet);

  /**
   * 包头部
   */
  uint8_t GetHeader();

  /**
   * 警告标志
   */
  uint16_t GetWarning();

  /**
   * 状态标志
   */
  uint16_t GetStatusFlags();

 private:
  struct Impl;

  std::unique_ptr<Impl> impl_;
};

}  // namespace Protocol
