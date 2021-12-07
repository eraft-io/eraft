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

#ifndef ERAFT_KV_RAFT_SERVER_H_
#define ERAFT_KV_RAFT_SERVER_H_

#include <eraftio/kvrpcpb.pb.h>
#include <eraftio/raft_cmdpb.pb.h>
#include <eraftio/raft_serverpb.pb.h>
#include <eraftio/tinykvpb.grpc.pb.h>
#include <kv/config.h>
#include <kv/engines.h>
#include <kv/node.h>
#include <kv/raft_store.h>
#include <kv/router.h>
#include <kv/storage.h>

#include <memory>

namespace kvserver {

class RegionReader : public StorageReader {
 public:
  RegionReader(std::shared_ptr<Engines> engs, metapb::Region region);

  ~RegionReader();

  std::string GetFromCF(std::string cf, std::string key);

  rocksdb::Iterator* IterCF(std::string cf);

  void Close();

 private:
  std::shared_ptr<Engines> engs_;

  metapb::Region region_;
};

class RaftStorage : public Storage {
 public:
  RaftStorage(std::shared_ptr<Config> conf);

  ~RaftStorage();

  bool CheckResponse(raft_cmdpb::RaftCmdResponse* resp, int reqCount);

  bool Write(const kvrpcpb::Context& ctx, const kvrpcpb::RawPutRequest* put);

  StorageReader* Reader(const kvrpcpb::Context& ctx);

  bool Raft(const raft_serverpb::RaftMessage* msg);

  bool SnapShot(raft_serverpb::RaftSnapshotData* snap);

  bool Start();

  std::shared_ptr<Engines> engs_;

  std::shared_ptr<RaftStore> raftSystem_;

 private:
  std::shared_ptr<Config> conf_;

  std::shared_ptr<Node> node_;

  std::shared_ptr<RaftRouter> raftRouter_;

  RegionReader* regionReader_;
};

}  // namespace kvserver

#endif  // ERAFT_KV_RAFT_SERVER_H_