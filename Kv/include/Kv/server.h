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

#ifndef ERAFT_KV_SERVER_IMPL_H_
#define ERAFT_KV_SERVER_IMPL_H_

#include <Kv/raft_server.h>
#include <Kv/storage.h>
#include <eraftio/metapb.pb.h>
#include <eraftio/raft_serverpb.pb.h>
#include <eraftio/tinykvpb.grpc.pb.h>
#include <grpcpp/grpcpp.h>

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <memory>

using grpc::ServerContext;
using grpc::Status;
using raft_serverpb::Done;
using tinykvpb::TinyKv;

namespace kvserver {

const std::string DEFAULT_ADDR = "127.0.0.1:12306";

class Server : public TinyKv::Service {
 public:
  Server();

  Server(std::string addr, RaftStorage* st);

  ~Server();

  bool RunLogic();

  Status Raft(ServerContext* context, const raft_serverpb::RaftMessage* request,
              Done* response) override;

  Status RawGet(ServerContext* context, const kvrpcpb::RawGetRequest* request,
                kvrpcpb::RawGetResponse* response) override;

  Status RawPut(ServerContext* context, const kvrpcpb::RawPutRequest* request,
                kvrpcpb::RawPutResponse* response) override;

  Status RawDelete(ServerContext* context,
                   const kvrpcpb::RawDeleteRequest* request,
                   kvrpcpb::RawDeleteResponse* response) override;

  Status RawScan(ServerContext* context, const kvrpcpb::RawScanRequest* request,
                 kvrpcpb::RawScanResponse* response) override;

  Status Snapshot(ServerContext* context,
                  const raft_serverpb::SnapshotChunk* request,
                  Done* response) override;

  Status TransferLeader(ServerContext* context,
                        const raft_cmdpb::TransferLeaderRequest* request,
                        raft_cmdpb::TransferLeaderResponse* response) override;

  Status PeerConfChange(ServerContext* context,
                        const raft_cmdpb::ChangePeerRequest* request,
                        raft_cmdpb::ChangePeerResponse* response) override;

  Status SplitRegion(ServerContext* context,
                     const raft_cmdpb::SplitRequest* request,
                     raft_cmdpb::SplitResponse* response) override;
  static std::map<int, std::condition_variable*> readyCondVars_;

  static std::mutex readyMutex_;

 private:
  std::string serverAddress_;

  RaftStorage* st_;
};

}  // namespace kvserver

#endif