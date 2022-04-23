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

#include <network/raft_peer_msg_handler.h>

namespace network {

RaftPeerMsgHandler::RaftPeerMsgHandler() {}
RaftPeerMsgHandler::~RaftPeerMsgHandler() {}

std::shared_ptr<storage::WriteBatch> RaftPeerMsgHandler::ProcessRequest(
    eraftpb::Entry *entry, raft_messagepb::RaftCmdRequest *msg,
    std::shared_ptr<storage::WriteBatch> wb) {}

void RaftPeerMsgHandler::ProcessConfChange(
    eraftpb::Entry *entry, eraftpb::ConfChange *cc,
    std::shared_ptr<storage::WriteBatch> wb) {}

void RaftPeerMsgHandler::ProcessSplitRegion(
    eraftpb::Entry *entry, metapb::Region *newRegion,
    std::shared_ptr<storage::WriteBatch> wb) {}

std::shared_ptr<storage::WriteBatch> RaftPeerMsgHandler::Process(
    eraftpb::Entry *entry, std::shared_ptr<storage::WriteBatch> wb) {}

void RaftPeerMsgHandler::HandleRaftReady() {
  // pop ready message, send to tran
  // ServerTransport::Send(std::shared_ptr<raft_serverpb::RaftMessage> msg)
}

void RaftPeerMsgHandler::HandleMsg(Msg m) {
  // msg type do something
}

void RaftPeerMsgHandler::ProposeRequest(std::string palyload) {}

void RaftPeerMsgHandler::ProposeRaftCommand(std::string playload) {}

void RaftPeerMsgHandler::OnTick() {}

void RaftPeerMsgHandler::StartTicker() {}

void RaftPeerMsgHandler::OnRaftBaseTick() {}

bool RaftPeerMsgHandler::OnRaftMsg(raft_messagepb::RaftMessage *msg) {}

bool RaftPeerMsgHandler::CheckMessage(raft_messagepb::RaftMessage *msg) {}

}  // namespace network
