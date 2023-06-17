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
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-06-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <grpcpp/grpcpp.h>
#include <time.h>

#include <iostream>

#include "eraftkv.grpc.pb.h"
#include "eraftkv.pb.h"
#include "util.h"

using eraftkv::ERaftKv;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define CTL_VERSION "v1.0.0"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << "Welcome to eraftkv-ctl, Copyright (c) 2023 ERaftGroup "
              << CTL_VERSION << std::endl;
    std::cout << "putkv: ./eraftkv-ctl [leader_address] put [key] [value]"
              << std::endl;
    std::cout << "getkv: ./eraftkv-ctl [leader_address] get [key]" << std::endl;
    std::cout << "addnode: ./eraftkv-ctl [leader_address] addnode [node id] "
                 "[node address]"
              << std::endl;
    std::cout
        << "removenode: ./eraftkv-ctl [leader_address] removenode [node id]"
        << std::endl;
    return 0;
  }

  std::string svr_addr = std::string(argv[1]);
  std::string cmd = std::string(argv[2]);
  auto        chan_ =
      grpc::CreateChannel(svr_addr, grpc::InsecureChannelCredentials());
  std::unique_ptr<ERaftKv::Stub> stub_(ERaftKv::NewStub(chan_));
  if (cmd == "put") {
    ClientContext               context;
    eraftkv::ClientOperationReq req;
    time_t                      time_in_sec;
    time(&time_in_sec);
    req.set_op_timestamp(static_cast<uint64_t>(time_in_sec));
    auto kv_pair = req.add_kvs();
    kv_pair->set_key(std::string(argv[3]));
    kv_pair->set_value(std::string(argv[4]));
    kv_pair->set_op_type(eraftkv::ClientOpType::Put);
    eraftkv::ClientOperationResp resp;
    auto status = stub_->ProcessRWOperation(&context, req, &resp);
    status.ok() ? std::cout << "put key ok~ " << std::endl
                : std::cout << " put key error! " << std::endl;
  } else if (cmd == "get") {
    ClientContext               context_get;
    eraftkv::ClientOperationReq req_get;
    time_t                      time_in_sec;
    time(&time_in_sec);
    req_get.set_op_timestamp(static_cast<uint64_t>(time_in_sec));
    auto kv_pair_ = req_get.add_kvs();
    kv_pair_->set_key(std::string(argv[3]));
    kv_pair_->set_op_type(eraftkv::ClientOpType::Get);
    eraftkv::ClientOperationResp resp_get;
    auto status_ = stub_->ProcessRWOperation(&context_get, req_get, &resp_get);
    if (status_.ok()) {
      std::cout << "get value " << resp_get.ops(0).value() << std::endl;
    }
  } else if (cmd == "bench") {
    auto key_size = stoi(std::string(argv[3]));
    auto val_size = stoi(std::string(argv[4]));
    auto count = stoi(std::string(argv[5]));
    for (size_t i = 0; i < count; i++) {
      ClientContext               context;
      eraftkv::ClientOperationReq req;
      time_t                      time_in_sec;
      time(&time_in_sec);
      req.set_op_timestamp(static_cast<uint64_t>(time_in_sec));
      auto kv_pair = req.add_kvs();
      kv_pair->set_key(StringUtil::RandStr(key_size));
      kv_pair->set_value(StringUtil::RandStr(val_size));
      kv_pair->set_op_type(eraftkv::ClientOpType::Put);
      eraftkv::ClientOperationResp resp;
      auto status = stub_->ProcessRWOperation(&context, req, &resp);
      status.ok() ? std::cout << "put key ok~ " << std::endl
                  : std::cout << " put key error! " << std::endl;
    }
  } else if (cmd == "addnode") {
    ClientContext                   context;
    eraftkv::ClusterConfigChangeReq req;
    req.set_change_type(eraftkv::ClusterConfigChangeType::AddServer);
    req.mutable_server()->set_id(stoi(std::string(argv[3])));
    req.mutable_server()->set_address(std::string(argv[4]));
    eraftkv::ClusterConfigChangeResp resp;
    auto status = stub_->ClusterConfigChange(&context, req, &resp);
    status.ok() ? std::cout << "add node ok~ " << std::endl
                : std::cout << " add node error! " << std::endl;
  } else if (cmd == "removenode") {
    ClientContext                   context;
    eraftkv::ClusterConfigChangeReq req;
    req.set_change_type(eraftkv::ClusterConfigChangeType::RemoveServer);
    req.mutable_server()->set_id(stoi(std::string(argv[3])));
    eraftkv::ClusterConfigChangeResp resp;
    auto status = stub_->ClusterConfigChange(&context, req, &resp);
    status.ok() ? std::cout << "remove node ok~ " << std::endl
                : std::cout << " remove node error! " << std::endl;
  }
  return 0;
}
