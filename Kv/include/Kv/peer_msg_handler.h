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

#ifndef ERAFT_KV_PEER_MSG_HANDLER_H_
#define ERAFT_KV_PEER_MSG_HANDLER_H_

#include <Kv/engines.h>
#include <Kv/peer.h>
#include <Kv/raft_store.h>
#include <eraftio/raft_cmdpb.pb.h>
#include <rocksdb/write_batch.h>

#include <functional>
#include <memory>

namespace kvserver {

static raft_cmdpb::RaftCmdRequest* NewAdminRequest(uint64_t regionID,
                                                   metapb::Peer* peer);

static raft_cmdpb::RaftCmdRequest* NewCompactLogRequest(uint64_t regionID,
                                                        metapb::Peer* peer,
                                                        uint64_t compactIndex,
                                                        uint64_t compactTerm);

class PeerMsgHandler {
 public:
  PeerMsgHandler(std::shared_ptr<Peer> peer,
                 std::shared_ptr<GlobalContext> ctx);

  ~PeerMsgHandler();

  void Handle(Proposal* p);

  std::shared_ptr<rocksdb::WriteBatch> ProcessRequest(
      eraftpb::Entry* entry, raft_cmdpb::RaftCmdRequest* msg,
      std::shared_ptr<rocksdb::WriteBatch> wb);

  void ProcessAdminRequest(eraftpb::Entry* entry,
                           raft_cmdpb::RaftCmdRequest* req,
                           std::shared_ptr<rocksdb::WriteBatch> wb);

  void ProcessConfChange(eraftpb::Entry* entry, eraftpb::ConfChange* cc,
                         std::shared_ptr<rocksdb::WriteBatch> wb);

  void ProcessSplitRegion(eraftpb::Entry* entry, metapb::Region* newregion,
                          std::shared_ptr<rocksdb::WriteBatch> wb);

  std::shared_ptr<rocksdb::WriteBatch> Process(
      eraftpb::Entry* entry, std::shared_ptr<rocksdb::WriteBatch> wb);

  void HandleRaftReady();

  void HandleMsg(Msg m);

  bool PreProposeRaftCommand(raft_cmdpb::RaftCmdRequest* req);

  void ProposeAdminRequest(raft_cmdpb::RaftCmdRequest* msg, Callback* cb);

  void ProposeRequest(kvrpcpb::RawPutRequest* put);

  void ProposeRaftCommand(kvrpcpb::RawPutRequest* put);

  void OnTick();

  void StartTicker();

  void OnRaftBaseTick();

  bool OnRaftMsg(raft_serverpb::RaftMessage* msg);

  bool ValidateRaftMessage(raft_serverpb::RaftMessage* msg);

  bool CheckMessage(raft_serverpb::RaftMessage* msg);

  std::shared_ptr<std::string> GetRequestKey(raft_cmdpb::Request* req);

  bool CheckStoreID(raft_cmdpb::RaftCmdRequest* req, uint64_t storeID);

  bool CheckTerm(raft_cmdpb::RaftCmdRequest* req, uint64_t term);

  bool CheckPeerID(raft_cmdpb::RaftCmdRequest* req, uint64_t peerID);

 private:
  std::shared_ptr<Peer> peer_;

  std::shared_ptr<GlobalContext> ctx_;
};

}  // namespace kvserver

#endif