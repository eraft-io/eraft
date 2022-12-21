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

#include <atomic>
#include <mutex>

#include "buffer.h"
#include "unbounded_buffer.h"

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
