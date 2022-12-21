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

#include "auth.h"

#include <algorithm>
#include <vector>

#include "common.h"

namespace Protocol {

struct Protocol::AuthPacket::Impl {
  /**
   * 字符编码
   *  charset in auth packet.
   */
  uint8_t charset;

  /**
   * 最大消息长度
   *  max packet size in auth packet.
   */
  uint32_t maxPacketSize;

  /**
   * 最大认证响应长度
   *  max auth response len in auth packet
   */
  uint8_t authResponseLen;

  /**
   *  客户端标志位
   *   clientFlags in auth packet
   */
  uint32_t clientFlags;

  /**
   * 客户端响应报文段
   * auth response in auth packet
   */
  std::vector<uint8_t> authResponse;

  /**
   * 插件名字
   *
   * plugin name in auth packet
   */
  std::string pluginName;

  /**
   * 数据库名字
   *
   * database name in auth packet
   */
  std::string database;

  /**
   * 认证用户名
   *
   * user name in auth packet
   */
  std::string user;
};

AuthPacket::AuthPacket() : impl_(new Impl()) {}

AuthPacket::~AuthPacket() {}

/**
 *
 * 141, 166, 15, 32, 0, 0, 0, 1, 33,
 * 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 * 109, 111, 99, 107, 0,
 * 0, 116, 101, 115, 116, 0,
 * 109, 121, 115, 113, 108, 95, 110, 97, 116, 105, 118, 101, 95, 112, 97, 115,
 * 115, 119, 111, 114, 100, 0
 *
 */

bool AuthPacket::UnPack(std::vector<uint8_t>& packet) {
  std::vector<uint8_t>::iterator seek = packet.begin();
  impl_->clientFlags = static_cast<uint32_t>(*seek) |
                       static_cast<uint32_t>(*(seek + 1) << 8) |
                       static_cast<uint32_t>(*(seek + 2) << 16) |
                       static_cast<uint32_t>(*(seek + 3) << 24);
  seek += 4;
  if ((impl_->clientFlags & CLIENT_PROTOCOL_41) == 0) {
    // TODO log error to file: auth.unpack: only support protocol 4.1
    return false;
  }
  impl_->maxPacketSize = static_cast<uint32_t>(*seek) |
                         static_cast<uint32_t>(*(seek + 1) << 8) |
                         static_cast<uint32_t>(*(seek + 2) << 16) |
                         static_cast<uint32_t>(*(seek + 3) << 24);
  seek += 4;
  impl_->charset = *seek;
  seek++;
  // string[23] reserved
  seek += 23;
  // username
  std::vector<uint8_t>::iterator userEnd = std::find(seek, packet.end(), 0x00);
  if (userEnd == packet.end()) {
    // TODO log the error
    return false;
  }
  impl_->user = std::string(seek, userEnd);
  seek = userEnd;
  // zero
  seek++;
  if ((impl_->clientFlags & CLIENT_SECURE_CONNECTION) > 0) {
    impl_->authResponseLen = *seek;
    seek++;
    if (impl_->authResponseLen > 0) {
      for (size_t i = 0; i < impl_->authResponseLen; i++) {
        impl_->authResponse.push_back(*seek);
        seek++;
      }
      seek += impl_->authResponseLen;
    }
  } else {
    impl_->authResponse = std::vector<uint8_t>(seek, seek + 20);
    seek += 20;
    // read zero
    seek++;
  }
  if ((impl_->clientFlags & CLIENT_CONNECT_WITH_DB) > 0) {
    // init db name
    std::vector<uint8_t>::iterator dbNameEnd =
        std::find(seek, packet.end(), 0x00);
    if (dbNameEnd == packet.end()) {
      return false;
    }
    impl_->database = std::string(seek, dbNameEnd);
    seek = dbNameEnd;
    // 0
    seek++;
  }
  if ((impl_->clientFlags & CLIENT_PLUGIN_AUTH) > 0) {
    std::vector<uint8_t>::iterator pluginNameEnd =
        std::find(seek, packet.end(), 0x00);
    if (pluginNameEnd == packet.end()) {
      return false;
    }
    impl_->pluginName = std::string(seek, pluginNameEnd);
    seek = pluginNameEnd;
  }
  if (impl_->pluginName != DEFAULT_AUTH_PLUGIN_NAME) {
    return false;
  }
}

std::string AuthPacket::GetDatabaseName() { return impl_->database; }

std::string AuthPacket::GetPluginName() { return impl_->pluginName; }

std::string AuthPacket::GetUserName() { return impl_->user; }

}  // namespace Protocol