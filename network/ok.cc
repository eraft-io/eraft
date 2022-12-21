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

#include "ok.h"

#include "common.h"
#include "protocol_buffer.h"

namespace Protocol {

struct OkPacket::Impl {
  uint8_t header;

  uint64_t affectedRows;

  uint64_t lastInsertID;

  uint16_t statusFlags;

  uint16_t warnings;
};

OkPacket::OkPacket() : impl_(new Impl()) {}

OkPacket::~OkPacket() {}

// e.g 0, 10, 0, 2, 0, 0, 0
std::vector<uint8_t> OkPacket::Pack(uint64_t affectedRows,
                                    uint64_t lastInsertID, uint16_t statusFlags,
                                    uint16_t warnings) {
  impl_->header = OK_PACKET;
  impl_->affectedRows = affectedRows;
  impl_->lastInsertID = lastInsertID;
  impl_->statusFlags = statusFlags;
  impl_->warnings = warnings;
  std::vector<uint8_t> packetOutput;
  WriteU8(packetOutput, OK_PACKET);
  WriteLenEncode(packetOutput, impl_->affectedRows);
  WriteLenEncode(packetOutput, impl_->lastInsertID);
  WriteU16(packetOutput, impl_->statusFlags);
  WriteU16(packetOutput, impl_->warnings);
  return packetOutput;
}

}  // namespace Protocol
