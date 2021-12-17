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

#ifndef ERAFT_SCHEDULER_CLIENT_H_
#define ERAFT_SCHEDULER_CLIENT_H_

#include <eraftio/schedulerpb.grpc.pb.h>
#include <eraftio/schedulerpb.pb.h>
#include <grpcpp/grpcpp.h>
#include <kv/config.h>
#include <kv/raft_client.h>

namespace kvserver {

class SchedulerClient {
 public:
  SchedulerClient(std::shared_ptr<Config> c);
  ~SchedulerClient();

  std::shared_ptr<RaftConn> GetConn(std::string addr, uint64_t regionID);

  bool GetMembers(std::string addr, schedulerpb::GetMembersRequest& erquest);

  bool GetRegion(std::string addr, schedulerpb::GetRegionRequest& request);

  bool AskSplit(std::string addr, schedulerpb::AskSplitRequest& request);

  bool RegionHeartbeat(std::string addr,
                       schedulerpb::RegionHeartbeatRequest& request);

 private:
  std::shared_ptr<Config> conf_;

  std::mutex mu_;

  std::map<std::string, std::shared_ptr<RaftConn> > conns_;

  std::map<uint64_t, std::string> addrs_;
};

}  // namespace kvserver

#endif  // scheduler_client.h