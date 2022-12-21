// Copyright 2022 The uhp-sql Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include <cstring>
#include <vector>

class UnboundedBuffer {
 public:
  UnboundedBuffer() : readPos_(0), writePos_(0) {}

  std::size_t PushDataAt(const void* pData, std::size_t nSize,
                         std::size_t offset = 0);
  std::size_t PushData(const void* pData, std::size_t nSize);
  std::size_t Write(const void* pData, std::size_t nSize);
  void AdjustWritePtr(std::size_t nBytes) { writePos_ += nBytes; }

  std::size_t PeekDataAt(void* pBuf, std::size_t nSize, std::size_t offset = 0);
  std::size_t PeekData(void* pBuf, std::size_t nSize);
  void AdjustReadPtr(std::size_t nBytes) { readPos_ += nBytes; }

  char* ReadAddr() { return &buffer_[readPos_]; }
  char* WriteAddr() { return &buffer_[writePos_]; }

  bool IsEmpty() const { return ReadableSize() == 0; }
  std::size_t ReadableSize() const { return writePos_ - readPos_; }
  std::size_t WriteableSize() const { return buffer_.size() - writePos_; }

  void Shrink(bool tight = false);
  void Clear();
  void Swap(UnboundedBuffer& buf);

  static const std::size_t MAX_BUFFER_SIZE;

 private:
  void _AssureSpace(std::size_t size);
  std::size_t readPos_;
  std::size_t writePos_;
  std::vector<char> buffer_;
};
