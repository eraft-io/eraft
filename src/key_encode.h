/**
 * @file key_encode.h
 * @author jay_jieliu@outlook.com
 * @brief
 * @version 0.1
 * @date 2023-07-24
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <string>
#include <vector>

#include "util.h"
#include "proto_parser.h"

/**
 * @brief
 *
 * | slot | user_key |
 * @return std::string
 */
static std::string EncodeStringKey(uint16_t key_slot, std::string user_key) {
  std::string dst;
  dst.push_back('*');
  dst.push_back('2');
  dst.append("\r\n");
  dst.push_back('$');
  dst.append(std::to_string(std::to_string(key_slot).size()));
  dst.append("\r\n");
  dst.append(std::to_string(key_slot));
  dst.append("\r\n");
  dst.push_back('$');
  dst.append(std::to_string(user_key.size()));
  dst.append("\r\n");
  dst.append(user_key);
  dst.append("\r\n");
  return dst;
}

/**
 * @brief decode an encode key
 *
 * @param enc_key
 * @param key_slot
 * @param user_key
 */
static void DecodeStringKey(std::string  enc_key,
                            uint16_t*    key_slot,
                            std::string* user_key) {
  ProtoParser parser_;
  const char *const end = enc_key.c_str() + enc_key.size();
  const char       *ptr = enc_key.c_str();
  parser_.ParseRequest(ptr, end);
  auto params = parser_.GetParams();
  *key_slot = static_cast<uint16_t>(std::stoi(params[0]));
  *user_key = params[1];
}

/**
 * @brief
 * | flag | expire | user val |
 * *3\r\n$[flags len]\r\n[flags]$[expire len]\r\n[expire]$[user val len]\r\n[user val]
 * @return std::string
 */
static std::string EncodeStringVal(uint32_t expire, std::string user_val) {
  std::string dst;
  dst.push_back('*');
  dst.push_back('3');
  dst.append("\r\n");
  dst.push_back('$');
  dst.push_back('1');
  dst.append("\r\n");
  dst.push_back('s');
  dst.append("\r\n");
  dst.push_back('$');
  dst.append(std::to_string(std::to_string(expire).size()));
  dst.append("\r\n");
  dst.append(std::to_string(expire));
  dst.append("\r\n");
  dst.push_back('$');
  dst.append(std::to_string(user_val.size()));
  dst.append("\r\n");
  dst.append(user_val);
  dst.append("\r\n");
  return dst;
}

/**
 * @brief decode an encode value
 *
 * @param enc_val
 * @param flag
 * @param expire
 * @param user_val
 */
static void DecodeStringVal(std::string*  enc_val,
                            char*        flag,
                            uint32_t*    expire,
                            std::string* user_val) {
  ProtoParser parser_;
  const char *const end = enc_val->c_str() + enc_val->size();
  const char       *ptr = enc_val->c_str();
  parser_.ParseRequest(ptr, end);
  auto params = parser_.GetParams();
  *flag = params[0].at(0);
  *expire = static_cast<uint32_t>(std::stoi(params[1]));
  *user_val = params[2];
}
