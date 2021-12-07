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

#ifndef ERAFT_SCHEDULER_SERVER_H_
#define ERAFT_SCHEDULER_SERVER_H_

#include <eraftio/schedulerpb.grpc.pb.h>
#include <eraftio/schedulerpb.pb.h>
#include <grpcpp/grpcpp.h>
#include <kv/raft_server.h>
#include <kv/storage.h>

using grpc::ServerContext;
using grpc::Status;
using schedulerpb::Scheduler;

namespace kvserver {

class ScheServer : public Scheduler::Service {
 public:
  ScheServer();
  ScheServer(std::string addr, RaftStorage* st);

  ~ScheServer();

  bool RunLogic();

  Status GetMembers(ServerContext* context,
                    const schedulerpb::GetMembersRequest* request,
                    schedulerpb::GetMembersResponse* response) override;

  Status Bootstrap(ServerContext* context,
                   const schedulerpb::BootstrapRequest* request,
                   schedulerpb::BootstrapResponse* response) override;

  Status IsBootstrapped(ServerContext* context,
                        const schedulerpb::IsBootstrappedRequest* request,
                        schedulerpb::IsBootstrappedResponse* response) override;

  Status AllocID(ServerContext* context,
                 const schedulerpb::AllocIDRequest* request,
                 schedulerpb::AllocIDResponse* response) override;

  Status GetStore(ServerContext* context,
                  const schedulerpb::GetStoreRequest* request,
                  schedulerpb::GetStoreResponse* response) override;

  Status PutStore(ServerContext* context,
                  const schedulerpb::PutStoreRequest* request,
                  schedulerpb::PutStoreResponse* response) override;

  Status GetAllStores(ServerContext* context,
                      const schedulerpb::GetAllStoresRequest* request,
                      schedulerpb::GetAllStoresResponse* response) override;

  Status StoreHeartbeat(ServerContext* context,
                        const schedulerpb::StoreHeartbeatRequest* request,
                        schedulerpb::StoreHeartbeatResponse* response) override;

  Status GetRegion(ServerContext* context,
                   const schedulerpb::GetRegionRequest* request,
                   schedulerpb::GetRegionResponse* response) override;

  Status GetPrevRegion(ServerContext* context,
                       const schedulerpb::GetRegionRequest* request,
                       schedulerpb::GetRegionResponse* response) override;

  Status GetRegionByID(ServerContext* context,
                       const schedulerpb::GetRegionByIDRequest* request,
                       schedulerpb::GetRegionResponse* response) override;

  Status ScanRegions(ServerContext* context,
                     const schedulerpb::ScanRegionsRequest* request,
                     schedulerpb::ScanRegionsResponse* response) override;

  Status AskSplit(ServerContext* context,
                  const schedulerpb::AskSplitRequest* request,
                  schedulerpb::AskSplitResponse* response) override;

  Status GetClusterConfig(
      ServerContext* context,
      const schedulerpb::GetClusterConfigRequest* request,
      schedulerpb::GetClusterConfigResponse* response) override;

  Status PutClusterConfig(
      ServerContext* context,
      const schedulerpb::PutClusterConfigRequest* request,
      schedulerpb::PutClusterConfigResponse* response) override;

  Status GetOperator(ServerContext* context,
                     const schedulerpb::GetOperatorRequest* request,
                     schedulerpb::GetOperatorResponse* response) override;

 private:
  std::string serverAddress_;

  RaftStorage* st_;
};

}  // namespace kvserver

#endif
