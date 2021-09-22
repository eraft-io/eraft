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

#include <Kv/raft_client.h>
#include <eraftio/raft_serverpb.pb.h>
#include <eraftio/tinykvpb.grpc.pb.h>
#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>

using raft_serverpb::Done;
using tinykvpb::TinyKv;

namespace kvserver {

RaftConn::RaftConn(std::string addr_, std::shared_ptr<Config>) {
  this->chan_ = grpc::CreateChannel(addr_, grpc::InsecureChannelCredentials());
}

RaftConn::~RaftConn() {}

std::shared_ptr<grpc::Channel> RaftConn::GetChan() { return this->chan_; }

RaftClient::RaftClient(std::shared_ptr<Config> c) { this->conf_ = c; }

RaftClient::~RaftClient() {}

std::shared_ptr<RaftConn> RaftClient::GetConn(std::string addr,
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

bool RaftClient::Send(uint64_t storeID, std::string addr,
                      raft_serverpb::RaftMessage& msg) {
  std::shared_ptr<RaftConn> conn = this->GetConn(addr, msg.region_id());
  std::unique_ptr<TinyKv::Stub> stub_(TinyKv::NewStub(conn->GetChan()));
  Done done;
  grpc::ClientContext context;
  auto status = stub_->Raft(&context, msg, &done);
  {
    std::lock_guard<std::mutex> lck(this->mu_);
    conns_.erase(addr);
  }
  if (!status.ok()) {
    SPDLOG_ERROR("send msg to peer " + addr + " error! ");
    return false;
  }
  return true;
}

bool RaftClient::TransferLeader(std::string addr,
                                raft_cmdpb::TransferLeaderRequest& request) {
  std::shared_ptr<RaftConn> conn = this->GetConn(addr, 1);
  std::unique_ptr<TinyKv::Stub> stub_(TinyKv::NewStub(conn->GetChan()));
  raft_cmdpb::TransferLeaderResponse response;
  grpc::ClientContext context;
  stub_->TransferLeader(&context, request, &response);
  return true;
}

bool RaftClient::PeerConfChange(std::string addr,
                                raft_cmdpb::ChangePeerRequest& request) {
  std::shared_ptr<RaftConn> conn = this->GetConn(addr, 1);
  std::unique_ptr<TinyKv::Stub> stub_(TinyKv::NewStub(conn->GetChan()));
  raft_cmdpb::ChangePeerResponse response;
  grpc::ClientContext context;
  stub_->PeerConfChange(&context, request, &response);
  return true;
}

bool RaftClient::SplitRegion(std::string addr,
                             raft_cmdpb::SplitRequest& request) {
  std::shared_ptr<RaftConn> conn = this->GetConn(addr, 1);
  std::unique_ptr<TinyKv::Stub> stub_(TinyKv::NewStub(conn->GetChan()));
  raft_cmdpb::SplitResponse response;
  grpc::ClientContext context;
  stub_->SplitRegion(&context, request, &response);
  return true;
}

bool RaftClient::PutRaw(std::string addr, kvrpcpb::RawPutRequest& request) {
  std::shared_ptr<RaftConn> conn = this->GetConn(addr, 1);
  std::unique_ptr<TinyKv::Stub> stub_(TinyKv::NewStub(conn->GetChan()));
  kvrpcpb::RawPutResponse response;
  grpc::ClientContext context;
  auto status = stub_->RawPut(&context, request, &response);
  return true;
}

std::string RaftClient::GetRaw(std::string addr,
                               kvrpcpb::RawGetRequest& request) {
  std::shared_ptr<RaftConn> conn = this->GetConn(addr, 1);
  std::unique_ptr<TinyKv::Stub> stub_(TinyKv::NewStub(conn->GetChan()));
  kvrpcpb::RawGetResponse response;
  grpc::ClientContext context;
  auto status = stub_->RawGet(&context, request, &response);
  return response.value();
}

}  // namespace kvserver
