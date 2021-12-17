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

#include <eraftio/schedulerpb.grpc.pb.h>
#include <eraftio/schedulerpb.pb.h>
#include <grpcpp/grpcpp.h>
#include <kv/scheduler_client.h>
#include <spdlog/spdlog.h>

namespace kvserver {

SchedulerClient::SchedulerClient(std::shared_ptr<Config> c) { this->conf_ = c; }
SchedulerClient::~SchedulerClient() {}

std::shared_ptr<RaftConn> SchedulerClient::GetConn(std::string addr,
                                                   uint64_t regionID) {
  if (this->conns_.find(addr) != this->conns_.end()) {
    return this->conns_[addr];
  }
  std::shared_ptr<RaftConn> newConn =
      std::make_shared<RaftConn>(addr, this->conf_);
  {
    std::lock_guard<std::mutex> lck(this->mu_);
    this->conns_[addr] = newConn;
  }
  return newConn;
}

bool SchedulerClient::GetMembers(std::string addr,
                                 schedulerpb::GetMembersRequest& erquest);

bool SchedulerClient::GetRegion(std::string addr,
                                schedulerpb::GetRegionRequest& request);

bool SchedulerClient::AskSplit(std::string addr,
                               schedulerpb::AskSplitRequest& request);

bool SchedulerClient::RegionHeartbeat(
    std::string addr, schedulerpb::RegionHeartbeatRequest& request);

}  // namespace kvserver