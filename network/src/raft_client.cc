// MIT License

// Copyright (c) 2022 eraft dev group

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

#include <network/raft_client.h>
#include <spdlog/spdlog.h>

namespace network {

RaftConn::RaftConn(std::string targetAddr_) {
  auto ipPortPair =
      RaftEncodeAssistant::GetInstance()->AddrStrToIpPort(targetAddr_);
  redisConnContext_ = redisConnect(ipPortPair.first, ipPortPair.second);
  if (redisConnContext_->err) {
    SPDLOG_ERROR("connect to " + targetAddr_ + " error!");
  }
}

RaftConn::~RaftConn() { redisFree(redisConnContext_); }

bool RaftConn::Send(raft_messagepb::RaftMessage &msg, std::string cmd) {
  std::string sendMsg = msg.SerializeAsString();
  redisReply *reply =
      redisCommand(redisConnContext_, "%s %s", cmd.c_str(), sendMsg.c_str());
  if (std::string(reply->str) != "OK") {
    SPDLOG_ERROR("send raftmessage error: " + std::string(reply->str));
    return false;
  }
  freeReplyObject(reply);
  return true;
}

std::shared_ptr<RaftConn> RaftClient::GetConn(std::string addr,
                                              uint64_t regionId) {
  if (this->conns_.find(addr) != this->conns_.end()) {
    return this->conns_[addr];
  }

  std::shared_ptr<RaftConn> newConn = std::make_shared<RaftConn>(addr);
  {
    std::lock_guard<std::mutex> lck(this->mu_);
    this->conns_[addr] = newConn;
  }
  return newConn;
}

redisContext *RaftConn::GetConnContext() { return redisConnContext_; }

RaftClient::RaftClient(std::shared_ptr<RaftConfig> conf) : conf_(conf) {}

RaftClient::~RaftClient() {}

bool RaftClient::Send(uint64_t storeId, std::string addr,
                      raft_messagepb::RaftMessage &msg) {
  std::shared_ptr<RaftConn> conn = this->GetConn(addr, msg.region_id());
  return conn->Send(msg, "pushraftmsg")
}

bool RaftClient::TransferLeader(std::string addr,
                                raft_messagepb::TransferLeaderRequest &req) {}

bool RaftClient::PeerConfChange(std::string addr,
                                raft_messagepb::ChangePeerRequest &req) {}

bool RaftClient::SplitRegion(std::string addr,
                             raft_messagepb::SplitRequest &req) {}

}  // namespace network
