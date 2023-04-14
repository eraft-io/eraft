#pragma once
#include <filesystem>
#include <vector>
#include <stdint.h>
#include <string>
#include <memory>
#include <cstring>
#include <iostream>

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

template <typename ...Args>
void TraceLog(Args&& ...args) {
  std::ostringstream stream;
  (stream << ... << std::forward<Args>(args)) << '\n';

  std::cout << stream.str();
}
