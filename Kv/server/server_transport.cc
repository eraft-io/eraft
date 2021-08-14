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

#include <Kv/server_transport.h>
#include <Kv/utils.h>
#include <Logger/logger.h>
#include <google/protobuf/text_format.h>

namespace kvserver {

ServerTransport::ServerTransport(std::shared_ptr<RaftClient> raftClient,
                                 std::shared_ptr<RaftRouter> raftRouter) {
  this->raftClient_ = raftClient;
  this->raftRouter_ = raftRouter;
}

ServerTransport::~ServerTransport() {}

bool ServerTransport::Send(std::shared_ptr<raft_serverpb::RaftMessage> msg) {
  auto storeID = msg->to_peer().store_id();
  auto addr = msg->to_peer().addr();
  this->SendStore(storeID, msg, addr);
  return true;
}

void ServerTransport::SendStore(uint64_t storeID,
                                std::shared_ptr<raft_serverpb::RaftMessage> msg,
                                std::string addr) {
  Logger::GetInstance()->DEBUG_NEW(
      "send to store id " + std::to_string(storeID) + " store addr " + addr,
      __FILE__, __LINE__, "ServerTransport::SendStore");
  this->WriteData(storeID, addr, msg);
}

void ServerTransport::Resolve(uint64_t storeID,
                              std::shared_ptr<raft_serverpb::RaftMessage> msg) {
  this->resolving_.erase(storeID);
}

void ServerTransport::WriteData(
    uint64_t storeID, std::string addr,
    std::shared_ptr<raft_serverpb::RaftMessage> msg) {
  this->raftClient_->Send(storeID, addr, *msg);
}

void ServerTransport::SendSnapshotSock(
    std::string addr, std::shared_ptr<raft_serverpb::RaftMessage> msg) {}

}  // namespace kvserver
