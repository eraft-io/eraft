// MIT License

// Copyright (c) 2021 eraft dev group

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

#ifndef ERAFT_UNBOUNDED_BUFFER_H_
#define ERAFT_UNBOUNDED_BUFFER_H_

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

#endif
