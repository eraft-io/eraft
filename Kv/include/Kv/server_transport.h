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

#ifndef ERAFT_KV_SERVER_TRANSPORT_H_
#define ERAFT_KV_SERVER_TRANSPORT_H_

#include <Kv/engines.h>
#include <Kv/raft_client.h>
#include <Kv/router.h>
#include <Kv/transport.h>

#include <map>
#include <memory>
#include <string>

namespace kvserver {

class RaftRouter;

class ServerTransport : public Transport {
 public:
  ServerTransport(std::shared_ptr<RaftClient> raftClient,
                  std::shared_ptr<RaftRouter> raftRouter);

  ~ServerTransport();

  bool Send(std::shared_ptr<raft_serverpb::RaftMessage> msg);

  void SendStore(uint64_t storeID,
                 std::shared_ptr<raft_serverpb::RaftMessage> msg,
                 std::string addr);

  void Resolve(uint64_t storeID,
               std::shared_ptr<raft_serverpb::RaftMessage> msg);

  void WriteData(uint64_t storeID, std::string addr,
                 std::shared_ptr<raft_serverpb::RaftMessage> msg);

  void SendSnapshotSock(std::string addr,
                        std::shared_ptr<raft_serverpb::RaftMessage> msg);

  void Flush();

 private:
  std::shared_ptr<RaftClient> raftClient_;

  std::shared_ptr<RaftRouter> raftRouter_;

  std::map<uint64_t, void*> resolving_;

  std::shared_ptr<Engines> engine_;
};

}  // namespace kvserver

#endif