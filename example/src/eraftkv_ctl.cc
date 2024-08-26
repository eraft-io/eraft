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
 * @file eraftkv_ctl.cc
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-06-10
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>
#include <time.h>

#include <iostream>

#include "client.h"
#include "eraft/util.h"
#include "protocol/eraftkv.grpc.pb.h"
#include "protocol/eraftkv.pb.h"

using eraftkv::ERaftKv;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define CTL_VERSION "v1.0.0"

enum op_code {
  QeuryGroups,
  AddGroup,
  SetSlot,
  RemoveGroup,
  PutKV,
  GetKV,
  RunBenchmark,
  Unknow
};

op_code hashit(std::string const& inString) {
  if (inString == "query_groups")
    return QeuryGroups;
  if (inString == "add_group")
    return AddGroup;
  if (inString == "set_slot")
    return SetSlot;
  if (inString == "remove_group")
    return RemoveGroup;
  if (inString == "put_kv")
    return PutKV;
  if (inString == "get_kv")
    return GetKV;
  if (inString == "run_bench")
    return RunBenchmark;
  return Unknow;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << "Welcome to eraftkv-ctl, Copyright (c) 2023 ERaftGroup "
              << CTL_VERSION << std::endl;
    std::cout
        << "put_kv: ./eraftkv-ctl [metaserver addresses] put_kv [key] [value]"
        << std::endl;
    std::cout << "get_kv: ./eraftkv-ctl [metaserver addresses] get_kv [key]"
              << std::endl;
    std::cout << "add_group: ./eraftkv-ctl [metaserver addresses] add_group "
                 "[groupid] [group server addresses]"
              << std::endl;
    std::cout << "remove_group: ./eraftkv-ctl [metaserver addresses] "
                 "remove_group [group id] [node id]"
              << std::endl;
    std::cout
        << "query_groups: ./eraftkv-ctl [metaserver addresses] query_groups"
        << std::endl;
    std::cout << "set_slot: ./eraftkv-ctl [metaserver addresses] set_slot "
                 "[groupid] [startSlot-endSlot]"
              << std::endl;
    exit(-1);
  }

  std::string metaserver_addrs = std::string(argv[1]);
  Client      eraftkv_ctl = Client(metaserver_addrs);

  std::string cmd = std::string(argv[2]);
  switch (hashit(cmd)) {
    case AddGroup: {
      int shard_id = stoi(std::string(argv[3]));
      eraftkv_ctl.AddServerGroupToMeta(shard_id, -1, std::string(argv[4]));
      break;
    }
    case QeuryGroups: {
      eraftkv_ctl.GetServerGroupsFromMeta();
      break;
    }
    case SetSlot: {
      auto slot_range_args = StringUtil::Split(std::string(argv[4]), '-');
      if (slot_range_args.size() == 2) {
        try {
          int64_t start_slot =
              static_cast<int64_t>(std::stoi(slot_range_args[0]));
          int64_t end_slot =
              static_cast<int64_t>(std::stoi(slot_range_args[1]));
          int shard_id = stoi(std::string(argv[3]));
          eraftkv_ctl.SetServerGroupSlotsToMeta(start_slot, end_slot, shard_id);
        } catch (const std::invalid_argument& e) {
          SPDLOG_ERROR("invalid_argument {}", e.what());
        }
      }
      break;
    }
    case RemoveGroup: {
      int shard_id = stoi(std::string(argv[3]));
      eraftkv_ctl.RemoveServerGroupFromMeta(shard_id);
      break;
    }
    case RunBenchmark: {
      int N = stoi(std::string(argv[3]));
      eraftkv_ctl.RunBench(N);
      break;
    }
    case PutKV: {
      auto partition_key = std::string(std::string(argv[3]));
      auto value = std::string(std::string(argv[4]));
      eraftkv_ctl.UpdateKvServerLeaderStubByPartitionKey(partition_key);
      eraftkv_ctl.PutKV(partition_key, value);
      break;
    }
    case GetKV: {
      eraftkv_ctl.UpdateKvServerLeaderStubByPartitionKey(std::string(argv[3]));
      eraftkv_ctl.GetKV(std::string(argv[3]));
      break;
    }
    default:
      break;
  }
  return 0;
}
