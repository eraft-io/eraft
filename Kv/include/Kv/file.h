// MIT License

// Copyright (c) 2021 Colin

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

#ifndef ERAFT_KV_FILE_H_
#define ERAFT_KV_FILE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

namespace kvserver {

class File {
 public:
  ~File() noexcept;
  File(const File&) = delete;
  File(File&& other) noexcept;
  File& operator=(const File&) = delete;
  File& operator=(File&& other) noexcept;

  static bool IsAbsolutePath(const std::string& path);

  static std::string GetExeImagePath();

  static std::string GetExeParentDirectory();

  static void ListDirectory(const std::string& directory,
                            std::vector<std::string>& list);

  static bool CreateDirectory(const std::string& directory);

  static bool DeleteDirectory(const std::string& directory);
};

}  // namespace kvserver

#endif