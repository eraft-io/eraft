/**
 * @file async_buffer.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-06-17
 *
 * @copyright Copyright (c) 2023
 *
 */


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

  std::mutex               backBufLock_;
  std::atomic<std::size_t> backBytes_;
  UnboundedBuffer          backBuf_;
};
