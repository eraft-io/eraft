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

#ifndef ERAFT_KV_UTIL_H_
#define ERAFT_KV_UTIL_H_

#include <Kv/engines.h>
#include <eraftio/eraftpb.pb.h>
#include <eraftio/raft_serverpb.pb.h>
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <rocksdb/db.h>
#include <rocksdb/write_batch.h>
#include <stdint.h>

#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace kvserver {

// assistant
class Assistant {
 protected:
  static Assistant* instance_;

 public:
  static Assistant* GetInstance() {
    if (instance_ == nullptr) {
      instance_ = new Assistant();
      return instance_;
    }
  }

  Assistant(){};

  ~Assistant(){};

  static const std::vector<uint8_t> kLocalPrefix;

  static const std::vector<uint8_t> kRegionRaftPrefix;

  static const std::vector<uint8_t> kRegionMetaPrefix;

  static const uint8_t kRegionRaftPrefixLen;

  static const uint8_t kRegionRaftLogLen;

  static const uint64_t kInvalidID;

  static const std::vector<uint8_t> kRaftLogSuffix;

  static const std::vector<uint8_t> kRaftStateSuffix;

  static const std::vector<uint8_t> kApplyStateSuffix;

  static const std::vector<uint8_t> kRegionStateSuffix;

  static const std::vector<uint8_t> MinKey;

  static const std::vector<uint8_t> MaxKey;

  static const std::vector<uint8_t> LocalMinKey;

  static const std::vector<uint8_t> LocalMaxKey;

  static const std::vector<uint8_t> RegionMetaMinKey;

  static const std::vector<uint8_t> RegionMetaMaxKey;

  static const std::vector<uint8_t> PrepareBootstrapKey;

  static const std::vector<uint8_t> StoreIdentKey;

  //
  //  CF declare
  //
  static const std::string CfDefault;
  static const std::string CfWrite;
  static const std::string CfLock;

  static const std::vector<std::string> CFs;

  static std::string VecToString(std::vector<uint8_t> in) {
    return std::string(in.begin(), in.end());
  }

  static std::vector<uint8_t> StringToVec(std::string in) {
    return std::vector<uint8_t>(in.begin(), in.end());
  }

  static void EncodeFixed8(char* dst, uint8_t value) {
    uint8_t* const buffer = reinterpret_cast<uint8_t*>(dst);
    std::memcpy(buffer, &value, sizeof(uint8_t));
  }

  static void EncodeFixed64(char* dst, uint64_t value) {
    uint8_t* const buffer = reinterpret_cast<uint8_t*>(dst);
    std::memcpy(buffer, &value, sizeof(uint64_t));
  }

  static uint8_t DecodeFixed8(const char* ptr) {
    const uint8_t* const buffer = reinterpret_cast<const uint8_t*>(ptr);
    uint8_t result;
    std::memcpy(&result, buffer, sizeof(uint8_t));
    return result;
  }

  static uint64_t DecodeFixed64(const uint8_t* buffer) {
    uint64_t result;

    std::memcpy(&result, buffer, sizeof(uint64_t));
    return result;
  }

  static void PutFixed8(std::string* dst, uint8_t value) {
    char buf[sizeof(value)];
    EncodeFixed8(buf, value);
    dst->append(buf, sizeof(buf));
  }

  static void PutFixed64(std::string* dst, uint64_t value) {
    char buf[sizeof(value)];
    EncodeFixed64(buf, value);
    dst->append(buf, sizeof(buf));
  }

  static std::string KeyWithCF(std::string cf, std::string key) {
    return cf + "_" + key;
  }

  static std::string GetCF(rocksdb::DB* db, std::string cf, std::string key) {
    std::string res;
    rocksdb::Status s =
        db->Get(rocksdb::ReadOptions(), KeyWithCF(cf, key), &res);
    if (s.ok()) {
      return res;
    } else {
      return "";
    }
  }

  static bool PutCF(std::unique_ptr<rocksdb::DB> db, std::string cf,
                    std::string key, std::string val) {
    return (db->Put(rocksdb::WriteOptions(), KeyWithCF(cf, key), val)).ok();
  }

  static rocksdb::Status GetMeta(rocksdb::DB* db, std::string key,
                                 google::protobuf::Message* msg) {
    std::string res;
    rocksdb::Status s = db->Get(rocksdb::ReadOptions(), key, &res);
    msg->ParseFromString(res);
    return s;
  }

  static bool PutMeta(rocksdb::DB* db, std::string key,
                      google::protobuf::Message& msg) {
    std::string val;
    // debug msg
    // std::string debugVal;
    // google::protobuf::TextFormat::PrintToString(msg, &debugVal);
    val = msg.SerializeAsString();
    auto status = db->Put(rocksdb::WriteOptions(), key, val);
    return status.ok();
  }

  static bool SetMeta(rocksdb::WriteBatch* batch, std::string key,
                      google::protobuf::Message& msg) {
    std::string val;
    // debug msg
    // std::string debugVal;
    // google::protobuf::TextFormat::PrintToString(msg, &debugVal);
    val = msg.SerializeAsString();
    return batch->Put(key, val).ok();
  }

  //
  // RegionPrefix: kLocalPrefix + kRegionRaftPrefix + regionID + suffix
  //
  static std::string MakeRegionPrefix(uint64_t regionID, uint8_t suffix) {
    std::string dst;
    PutFixed8(&dst, kLocalPrefix[0]);
    PutFixed8(&dst, kRegionRaftPrefix[0]);
    PutFixed64(&dst, regionID);
    PutFixed8(&dst, suffix);
    return dst;
  }

  //
  // RegionKey: kLocalPrefix + kRegionRaftPrefix + regionID + suffix + subID
  //

  static std::string MakeRegionKey(uint64_t regionID, uint8_t suffix,
                                   uint64_t subID) {
    std::string dst;
    PutFixed8(&dst, kLocalPrefix[0]);
    PutFixed8(&dst, kRegionRaftPrefix[0]);
    PutFixed64(&dst, regionID);
    PutFixed8(&dst, suffix);
    PutFixed64(&dst, subID);
    return dst;
  }

  //
  //  RegionRaftPrefixKey: kLocalPrefix + kRegionRaftPrefix + regionID
  //

  static std::string RegionRaftPrefixKey(uint64_t regionID) {
    std::string dst;
    PutFixed8(&dst, kLocalPrefix[0]);
    PutFixed8(&dst, kRegionRaftPrefix[1]);
    PutFixed64(&dst, regionID);
    return dst;
  }

  static std::string RaftLogKey(uint64_t regionID, uint64_t index) {
    return MakeRegionKey(regionID, kRaftLogSuffix[0], index);
  }

  static std::string RaftStateKey(uint64_t regionID) {
    return MakeRegionPrefix(regionID, kRaftStateSuffix[0]);
  }

  static std::string ApplyStateKey(uint64_t regionID) {
    return MakeRegionPrefix(regionID, kApplyStateSuffix[0]);
  }

  static bool IsRaftStateKey(std::vector<uint8_t> key) {
    assert(key.size() >= 2);
    return (key.size() == 11 && key[0] == kLocalPrefix[0] &&
            key[1] == kRegionRaftPrefix[0]);
  }

  static void DecodeRegionMetaKey(std::vector<uint8_t> key, uint64_t* regionID,
                                  uint8_t* suffix) {
    if ((RegionMetaMinKey.size() + 8 + 1) != (key.size())) {
      // TODO: log invalid region meta key length for key
      *regionID = 0;
      *suffix = 0;
      return;
    }
    if (!((key[0] == RegionMetaMinKey[0]) && (key[1] == RegionMetaMinKey[1]))) {
      // TODO: invalid region meta key prefix for key
      *regionID = 0;
      *suffix = 0;
      return;
    }
    *regionID = DecodeFixed64(&key[2]);
    *suffix = key[key.size() - 1];
  }

  static std::string RegionMetaPrefixKey(uint64_t regionID) {
    std::string dst;
    PutFixed8(&dst, kLocalPrefix[0]);
    PutFixed8(&dst, kRegionMetaPrefix[0]);
    PutFixed64(&dst, regionID);
    return dst;
  }

  // kLocalPrefix + kRegionMetaPrefix + regionID + kRegionStateSuffix
  static std::string RegionStateKey(uint64_t regionID) {
    std::string dst;
    PutFixed8(&dst, kLocalPrefix[0]);
    PutFixed8(&dst, kRegionMetaPrefix[0]);
    PutFixed64(&dst, regionID);
    PutFixed8(&dst, kRegionStateSuffix[0]);
    return dst;
  }

  /// RaftLogIndex gets the log index from raft log key generated by RaftLogKey.
  static uint64_t RaftLogIndex(std::vector<uint8_t> key) {
    if (key.size() != kRegionRaftLogLen) {
      // log key is not a valid raft log key
      return 0;
    }
    return DecodeFixed64(&key[kRegionRaftLogLen - 8]);
  }

  static raft_serverpb::RegionLocalState* GetRegionLocalState(
      rocksdb::DB* db, uint64_t regionId) {
    raft_serverpb::RegionLocalState* regionLocalState;
    GetMeta(db, RegionStateKey(regionId), regionLocalState);
    return regionLocalState;
  }

  static std::pair<raft_serverpb::RaftLocalState*, rocksdb::Status>
  GetRaftLocalState(rocksdb::DB* db, uint64_t regionId) {
    raft_serverpb::RaftLocalState* raftLocalState =
        new raft_serverpb::RaftLocalState();
    rocksdb::Status s = GetMeta(db, RaftStateKey(regionId), raftLocalState);
    return std::pair<raft_serverpb::RaftLocalState*, rocksdb::Status>(
        raftLocalState, s);
  }

  static std::pair<raft_serverpb::RaftApplyState*, rocksdb::Status>
  GetApplyState(rocksdb::DB* db, uint64_t regionId) {
    raft_serverpb::RaftApplyState* applyState =
        new raft_serverpb::RaftApplyState();
    rocksdb::Status s = GetMeta(db, ApplyStateKey(regionId), applyState);
    return std::pair<raft_serverpb::RaftApplyState*, rocksdb::Status>(
        applyState, s);
  }

  static eraftpb::Entry* GetRaftEntry(rocksdb::DB* db, uint64_t regionId,
                                      uint64_t idx) {
    eraftpb::Entry* entry = new eraftpb::Entry();
    GetMeta(db, RaftLogKey(regionId, idx), entry);
    return entry;
  }

  static const uint8_t kRaftInitLogTerm = 5;
  static const uint8_t kRaftInitLogIndex = 5;

  //
  //  This function init raft local state to db
  //
  static std::pair<raft_serverpb::RaftLocalState*, bool> InitRaftLocalState(
      rocksdb::DB* raftEngine, std::shared_ptr<metapb::Region> region) {
    auto lst = GetRaftLocalState(raftEngine, region->id());
    if (lst.second.IsNotFound()) {
      // raft_serverpb::RaftLocalState* raftState =
      //     new raft_serverpb::RaftLocalState();
      raft_serverpb::RaftLocalState* raftState = lst.first;
      raftState->set_allocated_hard_state(new eraftpb::HardState);
      // new split region
      if (region->peers().size() > 0) {
        raftState->set_last_index(kRaftInitLogIndex);
        raftState->set_last_term(kRaftInitLogTerm);
        raftState->mutable_hard_state()->set_term(kRaftInitLogTerm);
        raftState->mutable_hard_state()->set_commit(kRaftInitLogIndex);
        if (!PutMeta(raftEngine, RaftStateKey(region->id()), *raftState)) {
          return std::pair<raft_serverpb::RaftLocalState*, bool>(raftState,
                                                                 false);
        }
      }
    }
    return std::pair<raft_serverpb::RaftLocalState*, bool>(lst.first, true);
  }

  static std::pair<raft_serverpb::RaftApplyState*, bool> InitApplyState(
      rocksdb::DB* kvEngine, std::shared_ptr<metapb::Region> region) {
    auto ast = GetApplyState(kvEngine, region->id());
    if (ast.second.IsNotFound()) {
      raft_serverpb::RaftApplyState* applyState =
          new raft_serverpb::RaftApplyState();
      applyState->set_allocated_truncated_state(
          new raft_serverpb::RaftTruncatedState);
      if (region->peers().size() > 0) {
        applyState->set_applied_index(kRaftInitLogIndex);
        applyState->mutable_truncated_state()->set_index(kRaftInitLogIndex);
        applyState->mutable_truncated_state()->set_term(kRaftInitLogTerm);
      }
      if (!PutMeta(kvEngine, ApplyStateKey(region->id()), *applyState)) {
        return std::pair<raft_serverpb::RaftApplyState*, bool>(applyState,
                                                               false);
      }
    }
    return std::pair<raft_serverpb::RaftApplyState*, bool>(ast.first, false);
  }

  //
  // write region state to kv write batch
  //
  static void WriteRegionState(std::shared_ptr<rocksdb::WriteBatch> kvWB,
                               std::shared_ptr<metapb::Region> region,
                               const raft_serverpb::PeerState& state) {
    raft_serverpb::RegionLocalState* regionState =
        new raft_serverpb::RegionLocalState();
    regionState->set_state(state);
    regionState->set_allocated_region(region.get());
    SetMeta(kvWB.get(), RegionStateKey(region->id()), *regionState);
  }

  static bool DeleteCF(std::unique_ptr<rocksdb::DB> db, std::string cf,
                       std::string key) {
    return (db->Delete(rocksdb::WriteOptions(), KeyWithCF(cf, key))).ok();
  }

  static void DeleteRangeCF(std::shared_ptr<rocksdb::DB> db,
                            std::shared_ptr<rocksdb::WriteBatch> batch,
                            std::string cf, std::string startKey,
                            std::string endKey) {
    batch->DeleteRange(KeyWithCF(cf, startKey), KeyWithCF(cf, endKey));
  }

  static bool DeleteRange(std::shared_ptr<rocksdb::DB> db, std::string startKey,
                          std::string endKey) {
    std::shared_ptr<rocksdb::WriteBatch> batch;
    for (auto cf : CFs) {
      DeleteRangeCF(db, batch, cf, startKey, endKey);
    }
    return db->Write(rocksdb::WriteOptions(), &*batch).ok();
  }

  static rocksdb::Iterator* NewCFIterator(rocksdb::DB* db, std::string cf) {
    rocksdb::ReadOptions read_options;
    read_options.auto_prefix_mode = true;
    auto iter = db->NewIterator(read_options);
    iter->Seek(cf + "_");
    return iter;
  }

  static bool ExceedEndKey(std::string current, std::string endKey) {
    if (endKey.size() == 0) {
      return false;
    }
    return current.compare(endKey) >= 0;
  }

  static const uint64_t kRaftInvalidIndex = 0;

  static bool IsInitialMsg(eraftpb::Message& msg) {
    return msg.msg_type() == eraftpb::MsgRequestVote ||
           (msg.msg_type() == eraftpb::MsgHeartbeat &&
            msg.commit() == kRaftInvalidIndex);
  }

  static eraftpb::ConfState ConfStateFromRegion(
      std::shared_ptr<metapb::Region> region) {
    eraftpb::ConfState confState;
    for (auto p : region->peers()) {
      confState.add_nodes(p.id());
    }
    return confState;
  }

  static bool DoClearMeta(std::shared_ptr<Engines> engines,
                          rocksdb::WriteBatch* kvWB,
                          rocksdb::WriteBatch* raftWB, uint64_t regionID,
                          uint64_t lastIndex) {
    // TODO: stat cost time
    kvWB->Delete(RegionStateKey(regionID));
    kvWB->Delete(ApplyStateKey(regionID));

    uint64_t firstIndex = lastIndex + 1;
    std::string beginLogKey(RaftLogKey(regionID, 0));
    std::string endLogKey(RaftLogKey(regionID, firstIndex));

    auto iter = engines->raftDB_->NewIterator(rocksdb::ReadOptions());
    iter->Seek(beginLogKey);
    if (iter->Valid() && iter->key().ToString().compare(endLogKey) < 0) {
      uint64_t logIdx = RaftLogIndex(std::vector<uint8_t>{
          iter->key().ToString().begin(), iter->key().ToString().end()});
      firstIndex = logIdx;
    }

    for (uint64_t i = firstIndex; i <= lastIndex; i++) {
      raftWB->Delete(RaftLogKey(regionID, i));
    }

    raftWB->Delete(RaftStateKey(regionID));

    return true;
  }
};

}  // namespace kvserver

#endif
