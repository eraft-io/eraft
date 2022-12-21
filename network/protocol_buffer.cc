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

#include <unistd.h>

#include <iostream>
#include <limits>
#include <memory>
#include <vector>

namespace Protocol {

uint8_t ReadU8(std::vector<uint8_t>& packet,
               std::vector<uint8_t>::iterator& it) {
  // packet end check, TODO
  uint8_t v;
  v = *it;
  it++;
  return v;
}

uint16_t ReadU16(std::vector<uint8_t>& packet,
                 std::vector<uint8_t>::iterator& it) {
  // packet end check, TODO
  uint16_t v;
  v = static_cast<uint16_t>(*it) | static_cast<uint16_t>(*(it + 1) << 8);
  it += 2;
  return v;
}

uint16_t ReadU24(std::vector<uint8_t>& packet,
                 std::vector<uint8_t>::iterator& it) {
  uint32_t v;
  v = static_cast<uint32_t>(*it) | static_cast<uint32_t>(*(it + 1) << 8) |
      static_cast<uint32_t>(*(it + 2) << 16);
  it += 3;
  return v;
}

uint32_t ReadU32(std::vector<uint8_t>& packet,
                 std::vector<uint8_t>::iterator& it) {
  // packet end check, TODO
  uint32_t v;
  v = static_cast<uint32_t>(*it) | static_cast<uint32_t>(*(it + 1) << 8) |
      static_cast<uint32_t>(*(it + 2) << 16) |
      static_cast<uint32_t>(*(it + 3) << 24);
  it += 4;
  return v;
}

uint64_t ReadU64(std::vector<uint8_t>& packet,
                 std::vector<uint8_t>::iterator& it) {
  // packet end check, TODO
  uint64_t v;
  v = static_cast<uint64_t>(*it) | static_cast<uint64_t>(*(it + 1) << 8) |
      static_cast<uint64_t>(*(it + 2) << 16) |
      static_cast<uint64_t>(*(it + 3) << 24) |
      static_cast<uint64_t>(*(it + 4) << 32) |
      static_cast<uint64_t>(*(it + 5) << 40) |
      static_cast<uint64_t>(*(it + 6) << 48) |
      static_cast<uint64_t>(*(it + 7) << 56);
  it += 8;
  return v;
}

uint64_t ReadLenEncode(std::vector<uint8_t>& packet,
                       std::vector<uint8_t>::iterator& it) {
  uint8_t u8;
  uint16_t u16;
  uint32_t u24;
  uint64_t v;
  u8 = ReadU8(packet, it);
  switch (u8) {
    case 0xfb:
      // nil value
      return (uint64_t)std::numeric_limits<uint64_t>::max;
    case 0xfc:
      return static_cast<uint64_t>(ReadU16(packet, it));
    case 0xfd:
      return static_cast<uint64_t>(ReadU24(packet, it));
    case 0xfe:
      return static_cast<uint64_t>(ReadU64(packet, it));
    default:
      return static_cast<uint64_t>(u8);
  }
}

std::string ReadString(std::vector<uint8_t>& packet,
                       std::vector<uint8_t>::iterator& it, size_t readBytes) {
  // is it to end todo
  std::string readString = std::string(it, it + readBytes);
  it += readBytes;
  return readString;
}

std::string ReadLenEncodeString(std::vector<uint8_t>& packet,
                                std::vector<uint8_t>::iterator& it) {
  uint64_t strLen;
  strLen = ReadLenEncode(packet, it);
  return ReadString(packet, it, strLen);
}

void WriteU8(std::vector<uint8_t>& packet, uint8_t v) { packet.push_back(v); }

void WriteU16(std::vector<uint8_t>& packet, uint16_t v) {
  packet.push_back(static_cast<uint8_t>(v));
  packet.push_back(static_cast<uint8_t>(v >> 8));
}

void WriteU24(std::vector<uint8_t>& packet, uint32_t v) {
  packet.push_back(static_cast<uint8_t>(v));
  packet.push_back(static_cast<uint8_t>(v >> 8));
  packet.push_back(static_cast<uint8_t>(v >> 16));
}

void WriteU32(std::vector<uint8_t>& packet, uint32_t v) {
  packet.push_back(static_cast<uint8_t>(v));
  packet.push_back(static_cast<uint8_t>(v >> 8));
  packet.push_back(static_cast<uint8_t>(v >> 16));
  packet.push_back(static_cast<uint8_t>(v >> 24));
}

void WriteU64(std::vector<uint8_t>& packet, uint64_t v) {
  packet.push_back(static_cast<uint8_t>(v));
  packet.push_back(static_cast<uint8_t>(v >> 8));
  packet.push_back(static_cast<uint8_t>(v >> 16));
  packet.push_back(static_cast<uint8_t>(v >> 24));
  packet.push_back(static_cast<uint8_t>(v >> 32));
  packet.push_back(static_cast<uint8_t>(v >> 40));
  packet.push_back(static_cast<uint8_t>(v >> 48));
  packet.push_back(static_cast<uint8_t>(v >> 56));
}

void WriteLenEncode(std::vector<uint8_t>& packet, uint64_t v) {
  if (v < 251) {
    WriteU8(packet, static_cast<uint8_t>(v));
  } else if (v >= 251 && v < (1 << 16)) {
    WriteU8(packet, 0xfc);
    WriteU16(packet, static_cast<uint16_t>(v));
  } else if (v >= (1 << 16) && v < (1 << 24)) {
    WriteU8(packet, 0xfd);
    WriteU24(packet, static_cast<uint32_t>(v));
  } else {
    WriteU8(packet, 0xfe);
    WriteU64(packet, v);
  }
}

void WriteLenEncodeNUL(std::vector<uint8_t>& packet) { WriteU8(packet, 0xfb); }

void WriteString(std::vector<uint8_t>& packet, std::string v) {
  packet.insert(packet.end(), v.begin(), v.end());
}

void WriteLenEncodeString(std::vector<uint8_t>& packet, std::string v) {
  uint64_t len = v.size();
  WriteLenEncode(packet, len);
  WriteString(packet, v);
}

}  // namespace Protocol
