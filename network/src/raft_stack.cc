
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
#include <network/raft_config.h>
#include <network/raft_server_transport.h>
#include <network/raft_stack.h>

namespace network {

RaftStack::RaftStack(std::shared_ptr<RaftConfig> raftConf)
    : raftConf_(raftConf) {
  this->dbEngs_ = std::make_shared<DBEngines>(raftConf->dbPath_ + "_raft",
                                              raftConf->dbPath_ + "_kv");
}

RaftStack::~RaftStack() {}

bool RaftStack::Write(std::string playload) {}

bool RaftStack::Raft(const raft_messagepb::RaftMessage* msg) {
  return this->raftRouter_->SendRaftMessage(msg);
}

std::string RaftStack::Read() {}

bool RaftStack::Start() {
  // raft system init
  raftSystem_ = std::make_shared<RaftStore>();

  // router init
  raftRouter_ = this->raftSystem_->raftRouter_;

  // raft client init
  std::shared_ptr<RaftClient> raftClient =
      std::make_shared<RaftClient>(this->raftConf_);

  // raft node init
  raftNode_ = std::make_shared<RaftNode>(this->raftSystem_, this->raftConf_);

  // init server transport
  std::shared_ptr<RaftServerTransport> trans =
      std::make_shared<RaftServerTransport>(raftClient, raftRouter_);
  if (this->raftNode_->Start(this->dbEngs_, trans)) {
    SPDLOG_INFO("raftstack start succeed!");
  } else {
    SPDLOG_ERROR("raftstack start failed!");
  }
}
}  // namespace network
