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
  DirectoryTool(/* args */);
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


class RandomString {

 public:
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

  RandomString() = delete;
  ~RandomString() = delete;
};