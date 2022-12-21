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

#include "eof.h"

#include "common.h"
#include "protocol_buffer.h"

namespace Protocol {

struct EofPacket::Impl {
  /**
   * 包头部
   */
  uint8_t header;

  /**
   * 警告标志
   */
  uint16_t warnings;

  /**
   * 状态标志
   */
  uint16_t statusFlags;
};

EofPacket::EofPacket() : impl_(new Impl()) {}

EofPacket::~EofPacket() noexcept {}

EofPacket::EofPacket(uint16_t warnFlag, uint16_t statusFlag)
    : impl_(new Impl()) {
  impl_->header = EOF_PACKET;
  impl_->warnings = warnFlag;
  impl_->statusFlags = statusFlag;
}

std::vector<uint8_t> EofPacket::Pack() {
  std::vector<uint8_t> packetOutput;
  // EOF
  packetOutput.push_back(impl_->header);
  // warning
  packetOutput.push_back(static_cast<uint8_t>(impl_->warnings));
  packetOutput.push_back(static_cast<uint8_t>(impl_->warnings >> 8));
  // status
  packetOutput.push_back(static_cast<uint8_t>(impl_->statusFlags));
  packetOutput.push_back(static_cast<uint8_t>(impl_->statusFlags >> 8));
  return packetOutput;
}

bool EofPacket::UnPack(std::vector<uint8_t>& packet) {
  std::vector<uint8_t>::iterator seek = packet.begin();
  // header
  // impl_->header = *seek;
  // seek ++;
  impl_->header = Protocol::ReadU8(packet, seek);
  // warning
  impl_->warnings =
      static_cast<uint16_t>(*seek) | static_cast<uint16_t>(*(seek + 1) << 8);
  seek += 2;
  // status
  impl_->statusFlags =
      static_cast<uint16_t>(*seek) | static_cast<uint16_t>(*(seek + 1) << 8);
  return true;
}

/**
 * 包头部
 */
uint8_t EofPacket::GetHeader() { return impl_->header; }

/**
 * 警告标志
 */
uint16_t EofPacket::GetWarning() { return impl_->warnings; }

/**
 * 状态标志
 */
uint16_t EofPacket::GetStatusFlags() { return impl_->statusFlags; }

}  // namespace Protocol