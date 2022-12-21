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

#include "field.h"

#include <vector>

#include "common.h"
#include "protocol_buffer.h"
#include "type.h"

namespace Protocol {

struct FieldPacket::Impl {
  /**
   *  name of the field
   */
  std::string name;

  /**
   *  type of the field
   */
  uint32_t type;

  /**
   * Remaining fields from mysql C API.
   */
  std::string table;

  std::string orgTable;

  std::string dataBase;

  std::string orgName;

  uint32_t columnLength;

  uint32_t charset;

  uint32_t decimals;

  uint32_t flags;
};

FieldPacket::FieldPacket() : impl_(new Impl()) {}

FieldPacket::~FieldPacket() {}

FieldPacket::FieldPacket(std::string name, uint32_t type, std::string table,
                         std::string orgTable, std::string dataBase,
                         std::string orgName, uint32_t columnLength,
                         uint32_t charset, uint32_t decimals, uint32_t flags)
    : impl_(new Impl()) {
  impl_->name = name;
  impl_->type = type;
  impl_->table = table;
  impl_->orgTable = orgTable;
  impl_->dataBase = dataBase;
  impl_->orgName = orgName;
  impl_->columnLength = columnLength;
  impl_->charset = charset;
  impl_->decimals = decimals;
  impl_->flags = flags;
}

bool FieldPacket::UnPack(std::vector<uint8_t>& packet) {
  std::vector<uint8_t>::iterator seek = packet.begin();
  // CataLog is ignored, always set to "def", length and data
  std::string catalog = ReadLenEncodeString(packet, seek);
  std::cout << catalog << std::endl;
  // Schema
  impl_->dataBase = ReadLenEncodeString(packet, seek);
  // Table
  impl_->table = ReadLenEncodeString(packet, seek);
  // Org Table
  impl_->orgTable = ReadLenEncodeString(packet, seek);
  // Name
  impl_->name = ReadLenEncodeString(packet, seek);
  // Org Name
  impl_->orgName = ReadLenEncodeString(packet, seek);
  // length of fixed-length fields, skip
  ReadLenEncode(packet, seek);
  // character set
  impl_->charset = static_cast<uint32_t>(ReadU16(packet, seek));
  // column length
  impl_->columnLength = ReadU32(packet, seek);
  // type
  uint8_t t = ReadU8(packet, seek);
  impl_->flags = static_cast<uint32_t>(ReadU16(packet, seek));
  // convert mysql flag ...
  impl_->type = static_cast<uint32_t>(
      MySQLToType(static_cast<int64_t>(t), static_cast<int64_t>(impl_->flags)));

  impl_->decimals = static_cast<uint32_t>(ReadU8(packet, seek));
  return true;
}

std::vector<uint8_t> FieldPacket::Pack() {
  std::vector<uint8_t> outputPacket;
  // lenenc_str Catalog, always 'def'
  WriteLenEncodeString(outputPacket, "def");
  // Schema
  WriteLenEncodeString(outputPacket, impl_->dataBase);
  // Table
  WriteLenEncodeString(outputPacket, impl_->table);
  // Org Table
  WriteLenEncodeString(outputPacket, impl_->orgTable);
  // Name
  WriteLenEncodeString(outputPacket, impl_->name);
  // Org Name
  WriteLenEncodeString(outputPacket, impl_->orgName);
  // fixed length
  WriteLenEncode(outputPacket, static_cast<uint64_t>(0x0c));
  // charset
  WriteU16(outputPacket, static_cast<uint16_t>(impl_->charset));
  // column length
  WriteU32(outputPacket, impl_->columnLength);
  // type
  WriteU8(outputPacket, static_cast<uint8_t>(TYPE_TO_MYSQL[impl_->type].first));
  // flags
  WriteU16(outputPacket,
           static_cast<uint16_t>(TYPE_TO_MYSQL[impl_->type].second));
  // Decimals
  WriteU8(outputPacket, static_cast<uint8_t>(impl_->decimals));
  // filler
  WriteU16(outputPacket, static_cast<uint16_t>(0));
  return outputPacket;
}

std::string FieldPacket::ToString() {
  return "name: " + impl_->name + ", type: " + std::to_string(impl_->type) +
         ", table: " + impl_->table + ", orgTable: " + impl_->orgTable +
         ", dataBase: " + impl_->dataBase + ", orgName: " + impl_->orgName +
         ", columnLength: " + std::to_string(impl_->columnLength) +
         ", charset: " + std::to_string(impl_->charset) +
         ", decimals: " + std::to_string(impl_->decimals) +
         ", flags: " + std::to_string(impl_->flags);
}

struct RowPacket::Impl {
  // 表示一行数据
  std::vector<std::string> row;
};

RowPacket::RowPacket() : impl_(new Impl()) {}

RowPacket::~RowPacket() {}

std::vector<uint8_t> RowPacket::Pack() {
  std::vector<uint8_t> outPutPacket;
  for (auto str : impl_->row) {
    WriteLenEncodeString(outPutPacket, str);
  }
  return outPutPacket;
}

RowPacket::RowPacket(std::vector<std::string>& row) : impl_(new Impl()) {
  impl_->row = row;
}

bool RowPacket::UnPack(std::vector<uint8_t>& packet, size_t colCount) {
  std::vector<uint8_t>::iterator seek = packet.begin();
  for (size_t i = 0; i < colCount; i++) {
    std::string str = ReadLenEncodeString(packet, seek);
    impl_->row.push_back(str);
  }
  return true;
}

std::string RowPacket::ToString() {
  std::string res;
  for (auto str : impl_->row) {
    res += str;
    res += ' ';
  }
  return res;
}

}  // namespace Protocol
