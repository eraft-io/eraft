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

#ifndef ERAFT_RAFT_PEER_MSG_HANDLER_H_
#define ERAFT_RAFT_PEER_MSG_HANDLER_H_

#include <eraftio/eraftpb.pb.h>
#include <eraftio/raft_messagepb.pb.h>
#include <network/concurrency_msg_queue.h>
#include <network/raft_peer.h>
#include <network/raft_store.h>
#include <storage/engine_interface.h>

#include <memory>

namespace network {

class RaftPeerMsgHandler {
 public:
  RaftPeerMsgHandler(std::shared_ptr<Peer> peer,
                     std::shared_ptr<GlobalContext> ctx);
  ~RaftPeerMsgHandler();

  std::shared_ptr<storage::WriteBatch> ProcessRequest(
      eraftpb::Entry *entry, raft_messagepb::RaftCmdRequest *msg,
      std::shared_ptr<storage::WriteBatch> wb);

  void ProcessConfChange(eraftpb::Entry *entry, eraftpb::ConfChange *cc,
                         std::shared_ptr<storage::WriteBatch> wb);

  void ProcessSplitRegion(eraftpb::Entry *entry, metapb::Region *newRegion,
                          std::shared_ptr<storage::WriteBatch> wb);

  std::shared_ptr<storage::WriteBatch> Process(
      eraftpb::Entry *entry, std::shared_ptr<storage::WriteBatch> wb);

  void HandleRaftReady();

  void HandleMsg(Msg m);

  void ProposeRequest(std::string palyload);

  void ProposeRaftCommand(std::string playload);

  void OnTick();

  void StartTicker();

  void OnRaftBaseTick();

  bool OnRaftMsg(raft_messagepb::RaftMessage *msg);

  bool CheckMessage(raft_messagepb::RaftMessage *msg);

 private:
  std::shared_ptr<RaftPeer> peer_;

  std::shared_ptr<GlobalContext> ctx_;
};
}  // namespace network

#endif