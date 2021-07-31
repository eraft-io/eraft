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

#include <Kv/engines.h>
#include <Kv/raft_server.h>
#include <Kv/utils.h>
#include <Logger/logger.h>

#include <cassert>

namespace kvserver {

RaftStorage::RaftStorage(std::shared_ptr<Config> conf) {
  this->engs_ =
      std::make_shared<Engines>(conf->dbPath_ + "_raft", conf->dbPath_ + "_kv");
  this->conf_ = conf;
}

RaftStorage::~RaftStorage() {}

bool RaftStorage::CheckResponse(raft_cmdpb::RaftCmdResponse* resp,
                                int reqCount) {}

bool RaftStorage::Write(const kvrpcpb::Context& ctx,
                        const kvrpcpb::RawPutRequest* put) {
  raft_serverpb::RaftMessage* sendMsg = new raft_serverpb::RaftMessage();
  // send raft message
  sendMsg->set_data(put->SerializeAsString());
  sendMsg->set_region_id(1);
  sendMsg->set_raft_msg_type(raft_serverpb::RaftMsgClientCmd);

  return this->Raft(sendMsg);
}

StorageReader* RaftStorage::Reader(const kvrpcpb::Context& ctx) {
  metapb::Region region;
  RegionReader* regionReader = new RegionReader(this->engs_, region);
  this->regionReader_ = regionReader;
  return regionReader;
}

bool RaftStorage::Raft(const raft_serverpb::RaftMessage* msg) {
  return this->raftRouter_->SendRaftMessage(msg);
}

bool RaftStorage::SnapShot(raft_serverpb::RaftSnapshotData* snap) {}

bool RaftStorage::Start() {
  // raft system init
  this->raftSystem_ = std::make_shared<RaftStore>(this->conf_);
  // router init
  this->raftRouter_ = this->raftSystem_->raftRouter_;

  // raft client init
  std::shared_ptr<RaftClient> raftClient =
      std::make_shared<RaftClient>(this->conf_);

  this->node_ = std::make_shared<Node>(this->raftSystem_, this->conf_);

  // server transport init
  std::shared_ptr<ServerTransport> trans =
      std::make_shared<ServerTransport>(raftClient, raftRouter_);

  if (this->node_->Start(this->engs_, trans)) {
    Logger::GetInstance()->DEBUG_NEW("raft storage start succeed!", __FILE__,
                                     __LINE__, "RaftStorage::Start");
  } else {
    Logger::GetInstance()->DEBUG_NEW("err: raft storage start error!", __FILE__,
                                     __LINE__, "RaftStorage::Start");
  }
}

bool RaftStorage::Stop() {
  // stop worker
}

RegionReader::RegionReader(std::shared_ptr<Engines> engs,
                           metapb::Region region) {
  this->engs_ = engs;
  this->region_ = region;
}

RegionReader::~RegionReader() {}

std::string RegionReader::GetFromCF(std::string cf, std::string key) {
  return Assistant::GetInstance()->GetCF(this->engs_->kvDB_, cf, key);
}

rocksdb::Iterator* RegionReader::IterCF(std::string cf) {
  return Assistant::GetInstance()->NewCFIterator(this->engs_->kvDB_, cf);
}

void RegionReader::Close() {}

}  // namespace kvserver
