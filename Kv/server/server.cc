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

#include <Kv/bootstrap.h>
#include <Kv/raft_server.h>
#include <Kv/rate_limiter.h>
#include <Kv/server.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <spdlog/spdlog.h>

#include <condition_variable>
#include <iostream>

namespace kvserver {

Server::Server() { this->serverAddress_ = DEFAULT_ADDR; }

std::map<int, std::condition_variable*> Server::readyCondVars_;

std::mutex Server::readyMutex_;

Server::Server(std::string addr, RaftStorage* st)
    : serverAddress_(addr), st_(st) {}

Status Server::Raft(ServerContext* context,
                    const raft_serverpb::RaftMessage* request, Done* response) {
  if (this->st_->Raft(request)) {
    return Status::OK;
  } else {
    return Status::CANCELLED;
  }
  return Status::OK;
}

Status Server::RawGet(ServerContext* context,
                      const kvrpcpb::RawGetRequest* request,
                      kvrpcpb::RawGetResponse* response) {
  auto reader = this->st_->Reader(request->context());
  auto val = reader->GetFromCF(request->cf(), request->key());
  response->set_value(val);
  return Status::OK;
}

Status Server::RawPut(ServerContext* context,
                      const kvrpcpb::RawPutRequest* request,
                      kvrpcpb::RawPutResponse* response) {
  SPDLOG_INFO("handle raw put with key " + request->key() + " value " +
              request->value() + " cf " + request->cf() + " region id " +
              std::to_string(request->context().region_id()));

  // TODO: no waiting for ack from raft. Value is not yet committed so a
  // if (!RateLimiter::GetInstance()->TryGetToken()) {
  //   SPDLOG_WARN("put failed count : " +
  //               std::to_string(RateLimiter::GetInstance()->GetFailedCount()));
  //   return Status::CANCELLED;
  // }
  // subsequent GET on key may return old value
  kvrpcpb::RawPutRequest* req = const_cast<kvrpcpb::RawPutRequest*>(request);
  int reqId = BootHelper::MockSchAllocID();
  req->set_id(reqId);
  std::mutex mapMutex;
  {
    std::condition_variable* newVar = new std::condition_variable();
    std::lock_guard<std::mutex> lg(mapMutex);
    Server::readyCondVars_[reqId] = newVar;
  }
  if (!this->st_->Write(request->context(), req)) {
    SPDLOG_ERROR("st write error!");
    return Status::CANCELLED;
  }
  {
    std::unique_lock<std::mutex> ul(Server::readyMutex_);
    Server::readyCondVars_[reqId]->wait(ul, [] { return true; });
  }
  return Status::OK;
}

Status Server::TransferLeader(
    ServerContext* context, const ::raft_cmdpb::TransferLeaderRequest* request,
    raft_cmdpb::TransferLeaderResponse* response) {
  std::shared_ptr<raft_serverpb::RaftMessage> sendMsg =
      std::make_shared<raft_serverpb::RaftMessage>();
  // send raft message
  sendMsg->set_data(request->SerializeAsString());
  sendMsg->set_region_id(1);
  sendMsg->set_raft_msg_type(raft_serverpb::RaftTransferLeader);
  this->Raft(context, sendMsg.get(), nullptr);
  return Status::OK;
}

Status Server::PeerConfChange(ServerContext* context,
                              const raft_cmdpb::ChangePeerRequest* request,
                              raft_cmdpb::ChangePeerResponse* response) {
  std::shared_ptr<raft_serverpb::RaftMessage> sendMsg =
      std::make_shared<raft_serverpb::RaftMessage>();
  sendMsg->set_data(request->SerializeAsString());
  sendMsg->set_region_id(1);
  sendMsg->set_raft_msg_type(raft_serverpb::RaftConfChange);
  this->Raft(context, sendMsg.get(), nullptr);
  return Status::OK;
}

Status Server::SplitRegion(ServerContext* context,
                           const raft_cmdpb::SplitRequest* request,
                           raft_cmdpb::SplitResponse* response) {
  std::shared_ptr<raft_serverpb::RaftMessage> sendMsg =
      std::make_shared<raft_serverpb::RaftMessage>();
  sendMsg->set_data(request->SerializeAsString());
  sendMsg->set_region_id(1);
  sendMsg->set_raft_msg_type(raft_serverpb::RaftSplitRegion);
  this->Raft(context, sendMsg.get(), nullptr);
  return Status::OK;
}

Status Server::RawDelete(ServerContext* context,
                         const kvrpcpb::RawDeleteRequest* request,
                         kvrpcpb::RawDeleteResponse* response) {
  return Status::OK;
}

Status Server::RawScan(ServerContext* context,
                       const kvrpcpb::RawScanRequest* request,
                       kvrpcpb::RawScanResponse* response) {
  return Status::OK;
}

Status Server::Snapshot(ServerContext* context,
                        const raft_serverpb::SnapshotChunk* request,
                        Done* response) {
  return Status::OK;
}

bool Server::RunLogic() {
  Server service;
  grpc::EnableDefaultHealthCheckService(true);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(this->serverAddress_,
                           grpc::InsecureServerCredentials());

  builder.RegisterService(&service);

  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

  // rate limit
  // RateLimiter::GetInstance()->SetCapacity(5);

  SPDLOG_INFO("server listening on: " + this->serverAddress_);

  server->Wait();
}

Server::~Server() {}

}  // namespace kvserver
