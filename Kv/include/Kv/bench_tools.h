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

#include <Kv/raft_client.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace kvserver {

enum class BenchCmdType { KvRawPut, KvRawGet };

struct BenchResult {
  std::string cmdName;

  uint64_t avgQps;

  double avgLatecy;
};

class BenchTools {
 public:
  BenchTools();

  BenchTools(uint64_t clientNums, uint64_t connectionNums, BenchCmdType cmdType,
             uint64_t opCount, uint64_t testKeySizeInBytes,
             uint64_t testValuesSizeInBytes,
             std::shared_ptr<RaftClient> raftClient, std::string targetAddr);
  ~BenchTools();

  static BenchTools* GetInstance(uint64_t clientNums, uint64_t connectionNums,
                                 BenchCmdType cmdType, uint64_t opCount,
                                 uint64_t testKeySizeInBytes,
                                 uint64_t testValuesSizeInBytes,
                                 std::shared_ptr<RaftClient> raftClient,
                                 std::string targetAddr) {
    if (instance_ == nullptr) {
      instance_ = new BenchTools(clientNums, connectionNums, cmdType, opCount,
                                 testKeySizeInBytes, testValuesSizeInBytes,
                                 raftClient, targetAddr);
    }
    return instance_;
  }

  std::vector<std::pair<std::string, std::string>> GenRandomKvPair(
      uint64_t testOpCount, uint64_t testKeySizeInBytes,
      uint64_t testValuesSizeInBytes);

  std::string GenRandomLenString(uint64_t len);

  BenchResult RunBenchmarks();

 private:
  static BenchTools* instance_;

  BenchCmdType cmdType_;

  std::string targetAddr_;

  uint64_t clientNums_;

  uint64_t connectionNums_;

  uint64_t testOpCount_;

  uint64_t testKeySizeInBytes_;

  uint64_t testValuesSizeInBytes_;

  std::shared_ptr<RaftClient> raftClient_;
};

}  // namespace kvserver
#endif