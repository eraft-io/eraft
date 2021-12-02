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

#ifndef ERAFT_KV_BENCH_TOOLS_H
#define ERAFT_KV_BENCH_TOOLS_H

#include <kv/raft_client.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace kvserver {

enum class BenchCmdType { KvRawPut, KvRawGet };

class BenchResult {
 public:
  BenchResult(const std::string& cmd_name, double avg_qps, double avg_latency,
              uint32_t case_num, uint32_t key_num, uint32_t valid_key_num);

  std::string PrettyBenchResult() const;

 private:
  std::string cmd_name_;

  double avg_qps_;

  double avg_latency_;

  uint32_t case_num_;

  uint32_t key_num_;

  uint32_t valid_key_num_;
};

class BenchTools {
 public:
  BenchTools();

  BenchTools(uint64_t client_nums, uint64_t connection_nums,
             BenchCmdType cmd_type, uint64_t test_op_count,
             uint64_t test_key_size_in_bytes, uint64_t test_value_size_in_bytes,
             std::shared_ptr<RaftClient> raft_client,
             const std::string& target_addr);
  ~BenchTools();

  // static BenchTools* GetInstance(uint64_t clientNums, uint64_t
  // connectionNums,
  //                                BenchCmdType cmdType, uint64_t opCount,
  //                                uint64_t testKeySizeInBytes,
  //                                uint64_t testValuesSizeInBytes,
  //                                std::shared_ptr<RaftClient> raftClient,
  //                                std::string targetAddr) {
  //   if (instance_ == nullptr) {
  //     instance_ = new BenchTools(clientNums, connectionNums, cmdType,
  //     opCount,
  //                                testKeySizeInBytes, testValuesSizeInBytes,
  //                                raftClient, targetAddr);
  //   }
  //   return instance_;
  // }

  std::vector<std::pair<std::string, std::string>> GenRandomKvPair(
      uint64_t testOpCount, uint64_t testKeySizeInBytes,
      uint64_t testValuesSizeInBytes);

  std::string GenRandomLenString(uint64_t len);

  BenchResult RunBenchmarks();

 private:
  // static BenchTools* instance_;

  BenchCmdType cmd_type_;

  std::string target_addr_;

  uint64_t client_nums_;

  uint64_t connection_nums_;

  uint64_t test_op_count_;

  uint64_t test_key_size_in_bytes_;

  uint64_t test_value_size_in_bytes_;

  std::shared_ptr<RaftClient> raft_client_;
};

}  // namespace kvserver
#endif