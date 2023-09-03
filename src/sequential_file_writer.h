/**
 * @file sequential_file_writer.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-08-30
 *
 * @copyright Copyright (c) 2023
 *
 */

// MIT License

// Copyright (c) 2019 Isac Casapu

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

#pragma once

#include <fstream>
#include <string>

class SequentialFileWriter {
 public:
  SequentialFileWriter();
  SequentialFileWriter(SequentialFileWriter&&);
  SequentialFileWriter& operator=(SequentialFileWriter&&);

  // Open the file at the relative path 'name' for writing. On errors throw
  // std::system_error
  void OpenIfNecessary(const std::string& name);

  // Write data from a string. On errors throws an exception drived from
  // std::system_error This method may take ownership of the string. Hence no
  // assumption may be made about the data it contains after it returns.
  void Write(std::string& data);

  bool NoSpaceLeft() const {
    return m_no_space;
  }

 private:
  std::string   m_name;
  std::ofstream m_ofs;
  bool          m_no_space;
};
