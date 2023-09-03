/**
 * @file sequential_file_reader.cc
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

#include "sequential_file_reader.h"

#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <stdexcept>

namespace {

template <typename T>
class MMapPtr : public std::unique_ptr<T, std::function<void(T*)> > {
 public:
  MMapPtr(T* addr, size_t len, int fd = -1)
      : std::unique_ptr<T, std::function<void(T*)> >(addr, [len, fd](T* addr) {
        unmap_and_close(addr, len, fd);
      }) {}

  MMapPtr() : MMapPtr(nullptr, 0, -1) {}

  using std::unique_ptr<T, std::function<void(T*)> >::unique_ptr;
  using std::unique_ptr<T, std::function<void(T*)> >::operator=;

 private:
  static void unmap_and_close(const void* addr, size_t len, int fd) {
    if ((MAP_FAILED != addr) && (nullptr != addr) && (len > 0)) {
      munmap(const_cast<void*>(addr), len);
    }

    if (fd >= 0) {
      close(fd);
    }

    return;
  }
};
};  // Anonymous namespace

SequentialFileReader::SequentialFileReader(const std::string& file_name)
    : m_file_path(file_name), m_data(nullptr), m_size(0) {
  int fd = open(file_name.c_str(), O_RDONLY);
  if (-1 == fd) {
    SPDLOG_ERROR("Failed to open file {}", file_name);
  }

  // Ensure that fd will be closed if this method aborts at any point
  MMapPtr<const std::uint8_t> mmap_p(nullptr, 0, fd);

  struct stat st {};
  int         rc = fstat(fd, &st);
  if (-1 == rc) {
    SPDLOG_ERROR("Failed to read file size.");
  }
  m_size = st.st_size;
  if (m_size > 0) {
    void* const mapping =
        mmap(0, m_size, PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
    if (MAP_FAILED == mapping) {
      SPDLOG_ERROR("Failed to map the file into memory.");
    }

    // Close the file descriptor, and protect the newly acquired memory mapping
    // inside an object
    mmap_p = MMapPtr<const std::uint8_t>(
        static_cast<std::uint8_t*>(mapping), m_size, -1);
    // Inform the kernel we plan sequential access
    rc = posix_madvise(mapping, m_size, POSIX_MADV_SEQUENTIAL);
    if (-1 == rc) {
      SPDLOG_ERROR(
          "Failed to set intended access pattern useing posix_madvise().");
    }

    m_data.swap(mmap_p);
  }
}

void SequentialFileReader::Read(size_t max_chunk_size) {
  size_t bytes_read = 0;

  // Handle empty files. Note that m_data will likely be null, so we take care
  // not to access it.
  if (0 == m_size) {
    OnChunkAvailable("", 0);
    return;
  }

  while (bytes_read < m_size) {
    size_t bytes_to_read = std::min(max_chunk_size, m_size - bytes_read);

    // TODO: Here would be a good point to hint the kernel about the size of out
    // subsequent read, by using posix_madvise() to give the advice
    // POSIX_MADV_WILLNEED for the following max_chunk_size bytes after the ones
    // we are about to read now. Hopefully by the time we need them, they'll be
    // in the cache.

    OnChunkAvailable(m_data.get() + bytes_read, bytes_to_read);

    // If we implemented the optimisation suggested above, now would be the time
    // to set the advice POSIX_MADV_SEQUENTIAL for the data we have just
    // finished reading. Note we should not use POSIX_MADV_DONTNEED because
    // Linux ignores it (see the posix_madvise man page), and because multiple
    // concurrent reads could suffer from it.

    bytes_read += bytes_to_read;
  }
}


SequentialFileReader::SequentialFileReader(SequentialFileReader&&) = default;
SequentialFileReader& SequentialFileReader::operator=(SequentialFileReader&&) =
    default;
SequentialFileReader::~SequentialFileReader() = default;