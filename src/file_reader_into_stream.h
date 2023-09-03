/**
 * @file file_reader_into_stream.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-09-02
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

#include <spdlog/spdlog.h>

#include <cstdint>
#include <string>

#include "eraftkv.grpc.pb.h"
#include "eraftkv.pb.h"
#include "sequential_file_reader.h"
#include "sys/errno.h"

template <class StreamWriter>
class FileReaderIntoStream : public SequentialFileReader {
 public:
  FileReaderIntoStream(const std::string& filename,
                       std::int32_t       id,
                       StreamWriter&      writer)
      : SequentialFileReader(filename), m_writer(writer), m_id(id) {}

  using SequentialFileReader::SequentialFileReader;
  using SequentialFileReader::operator=;

 protected:
  virtual void OnChunkAvailable(const void* data, size_t size) override {
    auto fc = eraftkv::SSTFileContent();
    fc.set_content(data, size);
    if (!m_writer.Write(fc)) {
      SPDLOG_ERROR("The server aborted the connection.");
    }
  }

 private:
  StreamWriter& m_writer;
  std::uint32_t m_id;
};
