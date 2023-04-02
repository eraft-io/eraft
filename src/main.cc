// Copyright [2023] [ERaftGroup]

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

// 	http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
/**
 * @brief
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char** argv) {
  rocksdb::DB*     db;
  rocksdb::Options options;
  options.create_if_missing = true;
  rocksdb::Status status = rocksdb::DB::Open(options, "/tmp/testdb", &db);
  assert(status.ok());
  std::cout << "Hi, ERaftGroup!" << std::endl;
  delete db;
  std::cout << "Start grpc" << std::endl;
  ERaftKvServerOptions options_;
  options_.svr_addr = "0.0.0.0:50051";
  ERaftKvServer server(options_);
  // server.BuildAndRunRpcServer();
  return 0;
}
