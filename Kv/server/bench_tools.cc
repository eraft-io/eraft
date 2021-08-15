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
#include <eraftio/kvrpcpb.grpc.pb.h>
#include <unistd.h>

#include <chrono>
#include <ctime>
#include <iostream>
#include <thread>

namespace kvserver {
BenchTools* BenchTools::instance_ = nullptr;

BenchTools::BenchTools(uint64_t clientNums, uint64_t connectionNums,
                       BenchCmdType cmdType, uint64_t opCount,
                       uint64_t testKeySizeInBytes,
                       uint64_t testValuesSizeInBytes,
                       std::shared_ptr<RaftClient> raftClient,
                       std::string targetAddr) {
  this->clientNums_ = clientNums;
  this->connectionNums_ = connectionNums;
  this->cmdType_ = cmdType;
  this->testOpCount_ = opCount;
  this->testKeySizeInBytes_ = testKeySizeInBytes;
  this->testValuesSizeInBytes_ = testValuesSizeInBytes;
  this->raftClient_ = raftClient;
  this->targetAddr_ = targetAddr;
}

BenchTools::~BenchTools() {}

std::vector<std::pair<std::string, std::string>> BenchTools::GenRandomKvPair(
    uint64_t testOpCount, uint64_t testKeySizeInBytes,
    uint64_t testValuesSizeInBytes) {
  std::vector<std::pair<std::string, std::string>> genResult;
  for (uint64_t i = 0; i < testOpCount; i++) {
    genResult.push_back(std::make_pair<std::string, std::string>(
        GenRandomLenString(testKeySizeInBytes),
        GenRandomLenString(testValuesSizeInBytes)));
  }
  return genResult;
}

std::string BenchTools::GenRandomLenString(uint64_t len) {
  const std::string CHARACTERS =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  std::random_device random_device;
  std::mt19937 generator(random_device());
  std::uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

  std::string random_string;

  for (std::size_t i = 0; i < len; ++i) {
    random_string += CHARACTERS[distribution(generator)];
  }

  return random_string;
}

BenchResult BenchTools::RunBenchmarks() {
  BenchResult benchRes;
  auto testCases =
      GenRandomKvPair(this->testOpCount_, this->testKeySizeInBytes_,
                      this->testValuesSizeInBytes_);
  kvrpcpb::RawPutRequest request;
  for (auto testCase : testCases) {
    request.mutable_context()->set_region_id(1);
    request.set_cf("test_cf");
    request.set_key(testCase.first);
    request.set_value(testCase.second);
    std::cout << "set key: " << testCase.first << std::endl;
    std::cout << "set value: " << testCase.second << std::endl;

    this->raftClient_->PutRaw(this->targetAddr_, request);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  return benchRes;
}

}  // namespace kvserver