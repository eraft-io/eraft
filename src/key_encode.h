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

enum Flag {
  TYPE_STRING = 0x01,
  TYPE_HASH = 0x02,
  TYPE_LIST = 0x03,
  TYPE_SET = 0x04,
  TYPE_ZSET = 0x05,
};

/**
 * @brief
 *
 * | slot (2B) | user_key (n B) |
 * @return std::string
 */
static std::string EncodeStringKey(uint16_t key_slot, std::string user_key) {
  std::string dst;
  EncodeDecodeTool::PutFixed16(&dst, key_slot);
  dst.append(user_key);
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
  std::vector<uint8_t> enc_key_seq = {enc_key.begin(), enc_key.end()};
  *key_slot = EncodeDecodeTool::DecodeFixed16(&enc_key_seq[0]);
  *user_key = std::string(enc_key_seq.begin() + 2, enc_key_seq.end());
}

/**
 * @brief
 *
 * | flags (1B) | expire (4B) | user_value (nB) |
 * @return std::string
 */
static std::string EncodeStringVal(uint32_t expire, std::string user_val) {
  std::string dst;
  dst.append(Flag::TYPE_STRING, 1);
  EncodeDecodeTool::PutFixed32(&dst, expire);
  dst.append(user_val);
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
static void DecodeStringVal(std::string  enc_val,
                            uint8_t*     flag,
                            uint32_t*    expire,
                            std::string* user_val) {
  std::vector<uint8_t> enc_val_seq = {enc_val.begin(), enc_val.end()};
  *flag = enc_val_seq[0];
  *expire = EncodeDecodeTool::DecodeFixed32(&enc_val_seq[1]);
  *user_val = std::string(enc_val_seq.begin() + 5, enc_val_seq.end());
}
