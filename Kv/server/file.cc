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

#include <Kv/file.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>

#include <regex>
#include <string>

namespace kvserver {

bool File::IsAbsolutePath(const std::string& path) {
  static std::regex absolutePathRegex("[~/].*");
  return std::regex_match(path, absolutePathRegex);
}

bool File::DeleteDirectory(const std::string& directory) {
  std::string directoryWithSeparator(directory);
  if ((directoryWithSeparator.length() > 0) &&
      (directoryWithSeparator[directoryWithSeparator.length() - 1] != '/')) {
    directoryWithSeparator += '/';
  }
  DIR* dir = opendir(directory.c_str());
  if (dir != NULL) {
    struct dirent entry;
    struct dirent* entryBack;
    while (true) {
      if (readdir_r(dir, &entry, &entryBack)) {
        break;
      }
      if (entryBack == NULL) {
        break;
      }
      std::string name(entry.d_name);
      if ((name == ".") || (name == "..")) {
        continue;
      }
      std::string filePath(directoryWithSeparator);
      filePath += entry.d_name;
      if (entry.d_type == DT_DIR) {
        if (!DeleteDirectory(filePath.c_str())) {
          return false;
        }
      } else {
        if (unlink(filePath.c_str()) != 0) {
          return false;
        }
      }
    }
    (void)closedir(dir);
    return (rmdir(directory.c_str()) == 0);
  }
  return true;
}

}  // namespace kvserver
