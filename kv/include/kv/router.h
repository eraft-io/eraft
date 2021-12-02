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

#ifndef ERAFT_KV_ROUTER_H_
#define ERAFT_KV_ROUTER_H_

#include <eraftio/raft_cmdpb.pb.h>
#include <eraftio/raft_serverpb.pb.h>
#include <kv/concurrency_queue.h>
#include <kv/msg.h>
#include <kv/peer.h>
#include <kv/raft_worker.h>
#include <kv/server_transport.h>
#include <stdint.h>

#include <atomic>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace kvserver {

class Peer;

struct Msg;

struct PeerState_ {
  PeerState_(std::shared_ptr<Peer> peer) {
    this->closed_ = 0;
    this->peer_ = peer;
  }

  std::atomic<uint32_t> closed_;

  std::shared_ptr<Peer> peer_;
};

class Router {
  friend class RaftWorker;
  friend class ServerTransport;

 public:
  Router();

  std::shared_ptr<PeerState_> Get(uint64_t regionID);

  void Register(std::shared_ptr<Peer> peer);

  void Close(uint64_t regionID);

  bool Send(uint64_t regionID, Msg msg);

  void SendStore(Msg m);

  ~Router() {}

 protected:
  std::map<uint64_t, std::shared_ptr<PeerState_> > peers_;

 private:
};

class RaftRouter {
 public:
  virtual bool Send(uint64_t regionID, Msg m) = 0;

  virtual bool SendRaftMessage(const raft_serverpb::RaftMessage* msg) = 0;

  virtual bool SendRaftCommand(const kvrpcpb::RawPutRequest* put) = 0;
};

class RaftstoreRouter : public RaftRouter {
 public:
  RaftstoreRouter(std::shared_ptr<Router> r);

  ~RaftstoreRouter();

  bool Send(uint64_t regionID, Msg m) override;

  bool SendRaftMessage(const raft_serverpb::RaftMessage* msg) override;

  bool SendRaftCommand(const kvrpcpb::RawPutRequest* put) override;

 private:
  std::shared_ptr<Router> router_;

  std::mutex mtx_;

  static raft_serverpb::RaftMessage* raft_msg_;
};

}  // namespace kvserver

#endif