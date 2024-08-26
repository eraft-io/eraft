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
 * @file client.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2024-03-31
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <vector>

#include "eraft/util.h"
#include "protocol/eraftkv.grpc.pb.h"
#include "protocol/eraftkv.pb.h"

#define DEFAULT_METASERVER_ADDRS \
  "172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090"
#define DEFAULT_SHARD_MONITOR_ADDRS \
  "http://172.18.0.10:18088,http://172.18.0.11:18081,http://172.18.0.12:18082"

using eraftkv::ERaftKv;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class Client {

 public:
  Client();

  Client(std::string& metaserver_addrs);

  void UpdateMetaServerLeaderStub();

  void UpdateKvServerLeaderStubByPartitionKey(std::string partition_key);

  bool AddServerGroupToMeta(int64_t     group_shard_id,
                            int64_t     leader_id,
                            std::string group_server_addrs);

  std::map<int64_t, eraftkv::ShardGroup> GetServerGroupsFromMeta();

  bool RemoveServerGroupFromMeta(int64_t group_shard_id);

  bool SetServerGroupSlotsToMeta(int64_t start_slot,
                                 int64_t end_slot,
                                 int64_t group_shard_id);

  bool PutKV(std::string k, std::string v);

  std::pair<std::string, std::string> GetKV(std::string k);

  void RunBench(int64_t N);

  ~Client();

 private:
  std::vector<std::string> metaserver_addrs_;

  std::map<std::string, std::unique_ptr<ERaftKv::Stub> > meta_svr_stubs_;

  std::unique_ptr<ERaftKv::Stub> meta_leader_stub_;

  std::unique_ptr<ERaftKv::Stub> kv_leader_stub_;

  std::string client_id_;

  int64_t command_id_;
};
