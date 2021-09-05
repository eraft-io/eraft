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
// BenchTools* BenchTools::instance_ = nullptr;

BenchResult::BenchResult(const std::string& cmd_name, double avg_qps,
                         double avg_latency, uint32_t case_num,
                         uint32_t key_num, uint32_t valid_key_num)
    : cmd_name_(cmd_name),
      avg_qps_(avg_qps),
      avg_latency_(avg_latency),
      case_num_(case_num),
      key_num_(key_num),
      valid_key_num_(valid_key_num) {}

std::string BenchResult::PrettyBenchResult() const {
  std::string result;
  result += "cmd_name_= " + cmd_name_;
  result += " avg_qps_= " + std::to_string(avg_qps_);
  result += " avg_latency_= " + std::to_string(avg_latency_);
  result += " case_num_= " + std::to_string(case_num_);
  result += " key_num_= " + std::to_string(key_num_);
  result += " valid_key_num_= " + std::to_string(valid_key_num_);
  return result;
}

BenchTools::BenchTools(uint64_t client_nums, uint64_t connection_nums,
                       BenchCmdType cmd_type, uint64_t test_op_count,
                       uint64_t test_key_size_in_bytes,
                       uint64_t test_value_size_in_bytes,
                       std::shared_ptr<RaftClient> raft_client,
                       const std::string& target_addr)
    : client_nums_(client_nums),
      connection_nums_(connection_nums),
      cmd_type_(cmd_type),
      test_op_count_(test_op_count),
      test_key_size_in_bytes_(test_key_size_in_bytes),
      test_value_size_in_bytes_(test_value_size_in_bytes),
      raft_client_(raft_client),
      target_addr_(target_addr) {}

BenchTools::~BenchTools() {}

std::vector<std::pair<std::string, std::string>> BenchTools::GenRandomKvPair(
    uint64_t test_op_count, uint64_t test_key_size_in_bytes,
    uint64_t test_values_size_in_bytes) {
  std::vector<std::pair<std::string, std::string>> test_cases;
  for (uint64_t i = 0; i < test_op_count; i++) {
    test_cases.push_back(std::make_pair<std::string, std::string>(
        GenRandomLenString(test_key_size_in_bytes),
        GenRandomLenString(test_values_size_in_bytes)));
  }
  return test_cases;
}

std::string BenchTools::GenRandomLenString(uint64_t len) {
  const static std::string CHARACTERS =
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
  std::map<std::string, std::string>
      validate_kv;  // evaluate the correctness of raft
  std::vector<std::pair<std::string, std::string>> test_cases =
      GenRandomKvPair(this->test_op_count_, this->test_key_size_in_bytes_,
                      this->test_value_size_in_bytes_);

  kvrpcpb::RawPutRequest put_request;
  put_request.mutable_context()->set_region_id(1);
  put_request.set_cf("test_cf");

  auto start = std::chrono::system_clock::now();
  for (auto it = test_cases.begin(); it != test_cases.end(); ++it) {
    const std::string& key = it->first;
    const std::string& value = it->second;

    put_request.set_key(key);
    put_request.set_value(value);
    std::cout << "set key: " << key << '\n';
    std::cout << "set value: " << value << '\n';
    this->raft_client_->PutRaw(this->target_addr_, put_request);

    validate_kv[key] = value;
    // for test, 80ms << 100ms raft tick, we must limit speed of propose, for
    // optimization, we to batch propose client request
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed = end - start;

  std::cout << "start validation: ------------------------------------- \n";
  kvrpcpb::RawGetRequest get_request;
  get_request.mutable_context()->set_region_id(1);
  get_request.set_cf("test_cf");

  size_t key_num = validate_kv.size();
  size_t valid_key_num = 0;
  for (auto it = validate_kv.begin(); it != validate_kv.end(); ++it) {
    const std::string& key = it->first;
    const std::string& value = it->second;

    get_request.set_key(key);
    std::string value_raft =
        raft_client_->GetRaw(this->target_addr_, get_request);

    if (value == value_raft) {
      ++valid_key_num;
    } else {
      std::cout << "ERROR: " << __FILE__ << ' ' << __LINE__
                << " read incorrect value! "
                << " key= " << key << " value= " << value
                << " value_raft= " << value_raft << "\n";
    }
  }

  double avg_latency = elapsed.count() / this->test_op_count_;

  double avg_qps = this->test_op_count_ / elapsed.count();

  return BenchResult("", avg_qps, avg_latency, this->test_op_count_, key_num,
                     valid_key_num);
}

}  // namespace kvserver
