// MIT License

// Copyright (c) 2021 Colin

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

#include <Kv/raft_server.h>
#include <Kv/server.h>
#include <Logger/logger.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <iostream>

namespace kvserver {

Server::Server() { this->serverAddress_ = DEFAULT_ADDR; }

Server::Server(std::string addr, RaftStorage* st) {
  this->serverAddress_ = addr;
  this->st_ = st;
}

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
  Logger::GetInstance()->DEBUG_NEW(
      "handle raw put with key " + request->key() + " value " +
          request->value() + " cf " + request->cf() + " region id " +
          std::to_string(request->context().region_id()),
      __FILE__, __LINE__, "Server::RawPut");
  // TODO: no waiting for ack from raft. Value is not yet committed so a
  // subsequent GET on key may return old value
  if (!this->st_->Write(request->context(), request)) {
    Logger::GetInstance()->DEBUG_NEW("err: st write error!", __FILE__, __LINE__,
                                     "Server::RawPut");
    return Status::CANCELLED;
  }
  return Status::OK;
}

Status Server::TransferLeader(ServerContext* context,
                              const raft_cmdpb::TransferLeaderRequest* request,
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

Status PeerConfChange(::grpc::ClientContext* context,
                      const ::raft_cmdpb::ChangePeerRequest& request,
                      ::raft_cmdpb::ChangePeerResponse* response) {
  // 构造配置变更的消息，发送到 raft group
  std::shared_ptr<raft_serverpb::RaftMessage> sendMsg =
      std::make_shared<raft_serverpb::RaftMessage>();

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

  Logger::GetInstance()->DEBUG_NEW(
      "server listening on: " + this->serverAddress_, __FILE__, __LINE__,
      "Server::RunLogic");

  server->Wait();
}

Server::~Server() {}

}  // namespace kvserver
