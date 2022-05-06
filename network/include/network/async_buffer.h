// MIT License

// Copyright (c) 2022 eraft dev group

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

#ifndef ERAFT_ASYNC_BUFFER_H_
#define ERAFT_ASYNC_BUFFER_H_

#include <network/buffer.h>
#include <network/unbounded_buffer.h>

#include <atomic>
#include <mutex>

class AsyncBuffer {
 public:
  explicit AsyncBuffer(std::size_t size = 128 * 1024);
  ~AsyncBuffer();

  void Write(const void *data, std::size_t len);
  void Write(const BufferSequence &data);

  void ProcessBuffer(BufferSequence &data);
  void Skip(std::size_t size);

 private:
  Buffer buffer_;

  UnboundedBuffer tmpBuf_;

  std::mutex backBufLock_;
  std::atomic<std::size_t> backBytes_;
  UnboundedBuffer backBuf_;
};

#endif