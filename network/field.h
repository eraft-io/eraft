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

class FieldPacket {
 public:
  ~FieldPacket();
  FieldPacket(const FieldPacket&) = delete;
  FieldPacket(FieldPacket&&) = delete;
  FieldPacket& operator=(const FieldPacket&) = delete;
  FieldPacket& operator=(FieldPacket&&) = delete;
  FieldPacket();

  /**
   *  编码 Field 包
   */
  std::vector<uint8_t> Pack();

  FieldPacket(std::string name, uint32_t type, std::string table,
              std::string orgTable, std::string dataBase, std::string orgName,
              uint32_t columnLength, uint32_t charset, uint32_t decimals,
              uint32_t flags);
  /**
   *  解码 Field 包
   */
  bool UnPack(std::vector<uint8_t>& packet);

  std::string ToString();

 private:
  struct Impl;

  std::unique_ptr<Impl> impl_;
};

class RowPacket {
 public:
  ~RowPacket();
  RowPacket(const RowPacket&) = delete;
  RowPacket(RowPacket&&) = delete;
  RowPacket& operator=(const RowPacket&) = delete;
  RowPacket& operator=(RowPacket&&) = delete;
  RowPacket();

  std::string ToString();
  RowPacket(std::vector<std::string>& row);
  /**
   * 编码 Row
   */
  std::vector<uint8_t> Pack();

  /**
   *  解码 Row
   */
  bool UnPack(std::vector<uint8_t>& packet, size_t colCount);

 private:
  struct Impl;

  std::unique_ptr<Impl> impl_;
};

}  // namespace Protocol
