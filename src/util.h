// MIT License

// Copyright (c) 2023 ERaftGroup

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/**
 * @file util.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-05-21
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <stdint.h>

#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;
class DirectoryTool {
 public:
  static bool                     IsDir(const std::string dirpath);
  static bool                     MkDir(const std::string dirpath);
  static int                      StaticDirSize(const std::string dirpath);
  static std::vector<std::string> ListDirFiles(const std::string dirpath);
  static void                     DeleteDir(const std::string dirpath);
  DirectoryTool();
  ~DirectoryTool();
};


class EncodeDecodeTool {
 public:
  static void EncodeFixed64(char* dst, uint64_t value) {
    uint8_t* const buffer = reinterpret_cast<uint8_t*>(dst);
    std::memcpy(buffer, &value, sizeof(uint64_t));
  }

  static void PutFixed64(std::string* dst, uint64_t value) {
    char buf[sizeof(value)];
    EncodeFixed64(buf, value);
    dst->append(buf, sizeof(buf));
  }

  static uint64_t DecodeFixed64(const uint8_t* buffer) {
    uint64_t result;
    std::memcpy(&result, buffer, sizeof(uint64_t));
    return result;
  }

  static void EncodeFixed16(char* dst, uint16_t value) {
    uint8_t* const buffer = reinterpret_cast<uint8_t*>(dst);
    std::memcpy(buffer, &value, sizeof(uint16_t));
  }

  static void PutFixed16(std::string* dst, uint16_t value) {
    char buf[sizeof(value)];
    EncodeFixed16(buf, value);
    dst->append(buf, sizeof(buf));
  }

  static uint16_t DecodeFixed16(const uint8_t* buffer) {
    uint16_t result;
    std::memcpy(&result, buffer, sizeof(uint16_t));
    return result;
  }

  static void EncodeFixed32(char* dst, uint32_t value) {
    uint8_t* const buffer = reinterpret_cast<uint8_t*>(dst);
    std::memcpy(buffer, &value, sizeof(uint32_t));
  }

  static void PutFixed32(std::string* dst, uint32_t value) {
    char buf[sizeof(value)];
    EncodeFixed32(buf, value);
    dst->append(buf, sizeof(buf));
  }

  static uint32_t DecodeFixed32(const uint8_t* buffer) {
    uint32_t result;
    std::memcpy(&result, buffer, sizeof(uint32_t));
    return result;
  }
};

template <typename... Args>
void TraceLog(Args&&... args) {
  std::ostringstream stream;
  (stream << ... << std::forward<Args>(args)) << '\n';

  std::cout << stream.str();
}

/**
 * @brief
 *
 */
class RandomNumber {
 public:
  /**
   * @brief generator a uint64_t random number in (low, high)
   *
   * @param low
   * @param high
   * @return uint64_t
   */
  static uint64_t Between(uint64_t low, uint64_t high) {
    std::random_device                      random_device;
    std::mt19937                            random_engine(random_device());
    std::uniform_int_distribution<uint64_t> int64_distribution(low, high);
    return int64_distribution(random_engine);
  }

  RandomNumber() = delete;
  ~RandomNumber() = delete;
};


class StringUtil {

 public:
  /**
   * @brief
   *
   * @param generate a random string of length len
   * @return std::string
   */
  static std::string RandStr(int64_t len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
      tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return tmp_s;
  }

  static bool endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
  }

  static std::vector<std::string> Split(const std::string& str, char delim) {
    std::stringstream        ss(str);
    std::string              item;
    std::vector<std::string> elems;
    while (std::getline(ss, item, delim)) {
      if (!item.empty()) {
        elems.push_back(item);
      }
    }
    return elems;
  }
};

class HashUtil {
 public:
  static uint64_t CRC64(uint64_t crc, const char* s, uint64_t l);
};
