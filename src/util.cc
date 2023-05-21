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
 * @file util.cc
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-05-21
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "util.h"

bool DirectoryTool::IsDir(const std::string dirpath) {
  return fs::is_directory(dirpath);
};

/**
 * @brief
 *
 * @param dirpath
 * @return true
 * @return false
 */
bool DirectoryTool::MkDir(const std::string dirpath) {
  return fs::create_directories(dirpath);
}

/**
 * @brief
 *
 * @param dirpath
 * @return int size of all files in directory in bytes, return -1 if dirpath not
 * exist,
 */
int DirectoryTool::StaticDirSize(const std::string dirpath) {
  if (!fs::is_directory(dirpath)) {
    return -1;
  }
  int allfilessize = 0;
  for (auto const& dir_entry : fs::recursive_directory_iterator(dirpath)) {
    if (fs::is_regular_file(dir_entry)) {
      allfilessize += dir_entry.file_size();
    }
  }
  return allfilessize;
}

/**
 * @brief Recursive List files in directory
 *
 * @param dirpath
 * @return std::vector<std::string>
 */
std::vector<std::string> DirectoryTool::ListDirFiles(
    const std::string dirpath) {
  std::vector<std::string> filenames;
  if (!fs::is_directory(dirpath)) {
    return filenames;
  }
  for (auto const& dir_entry : fs::recursive_directory_iterator(dirpath)) {
    filenames.push_back(dir_entry.path());
  }
  return filenames;
}

/**
 * @brief Recursive delete directory and all files in the directory
 *
 * @param dirpath
 */
void DirectoryTool::DeleteDir(const std::string dirpath) {
  fs::remove_all(dirpath);
}

/**
 * @brief Construct a new Directory Tool::Directory Tool object
 *
 */
DirectoryTool::DirectoryTool() {}

/**
 * @brief Destroy the Directory Tool::Directory Tool object
 *
 */
DirectoryTool::~DirectoryTool() {}
