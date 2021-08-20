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

#include <Kv/bench_tools.h>
#include <Kv/config.h>
#include <Kv/raft_client.h>
#include <eraftio/helloworld.grpc.pb.h>
#include <eraftio/kvrpcpb.grpc.pb.h>
#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>

using namespace kvserver;

int main(int argc, char** argv) {
  std::string target_addr(argv[1]);
  uint64_t op_count = std::atoi(argv[2]);
  uint64_t key_size = std::atoi(argv[3]);
  uint64_t value_size = std::atoi(argv[4]);

  std::shared_ptr<Config> conf =
      std::make_shared<Config>(target_addr, "/tmp/data", 0);
  std::shared_ptr<RaftClient> raft_client = std::make_shared<RaftClient>(conf);
  std::shared_ptr<BenchTools> bench_tools = std::make_shared<BenchTools>(
      1, 1, kvserver::BenchCmdType::KvRawPut, op_count, key_size, value_size,
      raft_client, target_addr);

  BenchResult benchmark_result = bench_tools->RunBenchmarks();
  std::cout << benchmark_result.PrettyBenchResult() << std::endl;
  return 0;
}
