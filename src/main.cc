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
 * @file main.cc
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <rocksdb/db.h>

#include <iostream>

#include "eraftkv_server.h"
#include "raft_server.h"
/**
 * @brief
 *
 * @param argc
 * @param argv (eg: eraftkv 0 /tmp/kv_db0 /tmp/log_db0 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090)
 * @return int
 */
int main(int argc, char* argv[]) {
  ERaftKvServerOptions options_;
  options_.svr_id = stoi(std::string(argv[1]));
  options_.kv_db_path = std::string(argv[2]);
  options_.log_db_path = std::string(argv[3]);
  options_.peer_addrs = std::string(argv[4]);
  ERaftKvServer server(options_);
  server.BuildAndRunRpcServer();
  return 0;
}
