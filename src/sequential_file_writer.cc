/**
 * @file sequential_file_writer.cc
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

#include "sequential_file_writer.h"

#include <spdlog/spdlog.h>
#include <sys/errno.h>

#include <cstdio>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

SequentialFileWriter::SequentialFileWriter() : m_no_space(false) {}

SequentialFileWriter::SequentialFileWriter(SequentialFileWriter&&) = default;
SequentialFileWriter& SequentialFileWriter::operator=(SequentialFileWriter&&) =
    default;

// Currently the implementation is very simple using standard library
// facilities. More advanced implementations allowing for better parallelism are
// possible, e.g. using aio_write().

void SequentialFileWriter::OpenIfNecessary(const std::string& name) {
  // FIXME: Sanitise file names. Currently there's nothing preventing the user
  // from giving absolute paths, Paths with .. etc. We should accept simple
  // relative paths only.

  if (m_ofs.is_open()) {
    return;
  }

  using std::ios_base;
  std::ofstream ofs;
  ofs.exceptions(std::ifstream::failbit | std::ifstream::badbit);

  try {
    ofs.open(name, ios_base::out | ios_base::trunc | ios_base::binary);
  } catch (const std::system_error& ex) {
    SPDLOG_ERROR("Opening {}", ex.what());
  }

  m_ofs = std::move(ofs);
  m_name = name;
  m_no_space = false;
  return;
}

void SequentialFileWriter::Write(std::string& data) {
  try {
    m_ofs << data;
  } catch (const std::system_error& ex) {
    if (m_ofs.is_open()) {
      m_ofs.close();
    }
    std::remove(m_name.c_str());  // Best effort. We expect it to succeed, but
                                  // we don't check whether it did
    SPDLOG_ERROR("Writing {}", ex.what());
  }

  data.clear();
  return;
}
