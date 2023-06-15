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
 * @file thread_pool.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-06-14
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <boost/asio.hpp>
#include <memory>
#include <thread>
#include <vector>

class AsioThreadPool {
 public:
  AsioThreadPool(int threadNum = std::thread::hardware_concurrency())
      : work_(new boost::asio::io_service::work(service_)) {
    for (int i = 0; i < threadNum; ++i) {
      threads_.emplace_back([this]() { service_.run(); });
    }
  }

  AsioThreadPool(const AsioThreadPool &) = delete;

  AsioThreadPool &operator=(const AsioThreadPool &) = delete;

  boost::asio::io_service &GetIOService() {
    return service_;
  }

  void Stop() {
    work_.reset();
    for (auto &t : threads_) {
      t.join();
    }
  }

 private:
  boost::asio::io_service                        service_;
  std::unique_ptr<boost::asio::io_service::work> work_;
  std::vector<std::thread>                       threads_;
};
