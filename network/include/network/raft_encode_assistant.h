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

#ifndef ERAFT_NETWORK_RAFT_ENCODE_ASSISTANT_H_
#define ERAFT_NETWORK_RAFT_ENCODE_ASSISTANT_H_

#include <eraftio/raft_messagepb.pb.h>
#include <storage/engine_interface.h>

#include <cstdint>
#include <vector>

namespace network {

class RaftEncodeAssistant {
 protected:
  static RaftEncodeAssistant *instance_;

 public:
  RaftEncodeAssistant();
  ~RaftEncodeAssistant() { delete instance_; }

  static RaftEncodeAssistant *GetInstance() {
    if (instance_ == nullptr) {
      instance_ = new RaftEncodeAssistant();
      return instance_;
    }
  }

  static const std::vector<uint8_t> kLocalPrefix;

  static std::string LocalPrefixStr() {
    return std::string(kLocalPrefix.begin(), kLocalPrefix.end());
  }

  static const std::vector<uint8_t> kRegionRaftPrefix;

  static std::string RegionRaftPrefixStr() {
    return std::string(kRegionRaftPrefix.begin(), kRegionRaftPrefix.end());
  }

  static const std::vector<uint8_t> kRegionMetaPrefix;

  static std::string RegionMetaPrefixStr() {
    return std::string(kRegionMetaPrefix.begin(), kRegionMetaPrefix.end());
  }

  static const uint8_t kRegionRaftPrefixLen;

  static const uint8_t kRegionRaftLogLen;

  static const uint64_t kInvalidID;

  static const std::vector<uint8_t> kRaftLogSuffix;

  static std::string RaftLogSuffixStr() {
    return std::string(kRaftLogSuffix.begin(), kRaftLogSuffix.end());
  }

  static const std::vector<uint8_t> kRaftStateSuffix;

  static std::string RaftStateSuffixStr() {
    return std::string(kRaftStateSuffix.begin(), kRaftStateSuffix.end());
  }

  static const std::vector<uint8_t> kApplyStateSuffix;

  static std::string ApplyStateSuffixStr() {
    return std::string(kApplyStateSuffix.begin(), kApplyStateSuffix.end());
  }

  static const std::vector<uint8_t> kRegionStateSuffix;

  static std::string RegionStateSuffixStr() {
    return std::string(kRegionStateSuffix.begin(), kRegionStateSuffix.end());
  }

  static std::pair<std::string, uint16_t> AddrStrToIpPort(std::string addr) {
    std::string::size_type p = addr.find_first_of(':');
    std::string ip = addr.substr(0, p);
    std::string port = addr.substr(p + 1);
    uint16_t portInt = static_cast<uint16_t>(std::stoi(port));
    return std::make_pair<std::string, uint16_t>(ip, portInt);
  }

  static const std::vector<uint8_t> MinKey;

  static std::string MinKeyStr() {
    return std::string(MinKey.begin(), MinKey.end());
  }

  static const std::vector<uint8_t> MaxKey;

  static std::string MaxKeyStr() {
    return std::string(MaxKey.begin(), MaxKey.end());
  }

  static const std::vector<uint8_t> LocalMinKey;

  static std::string LocalMinKeyStr() {
    return std::string(LocalMinKey.begin(), LocalMinKey.end());
  }

  static const std::vector<uint8_t> LocalMaxKey;

  static std::string LocalMaxKeyStr() {
    return std::string(LocalMaxKey.begin(), LocalMaxKey.end());
  }

  static const std::vector<uint8_t> RegionMetaMinKey;

  static std::string RegionMetaMinKeyStr() {
    return std::string(RegionMetaMinKey.begin(), RegionMetaMinKey.end());
  }

  static const std::vector<uint8_t> RegionMetaMaxKey;

  static std::string RegionMetaMaxKeyStr() {
    return std::string(RegionMetaMaxKey.begin(), RegionMetaMaxKey.end());
  }

  static const std::vector<uint8_t> PrepareBootstrapKey;

  static std::string PrepareBootstrapKeyStr() {
    return std::string(PrepareBootstrapKey.begin(), PrepareBootstrapKey.end());
  }

  static const std::vector<uint8_t> StoreIdentKey;

  static std::string StoreIdentKeyStr() {
    return std::string(StoreIdentKey.begin(), StoreIdentKey.end());
  }

  static const std::string CfDefault;

  static const std::string CfWrite;

  static const std::string CfLock;

  static std::string VecToString(std::vector<uint8_t> in) {
    return std::string(in.begin(), in.end());
  }

  static std::vector<uint8_t> StringToVec(std::string in) {
    return std::vector<uint8_t>(in.begin(), in.end());
  }

  static void EncodeFixed8(char *dst, uint8_t value) {
    uint8_t *const buffer = reinterpret_cast<uint8_t *>(dst);
    std::memcpy(buffer, &value, sizeof(uint8_t));
  }

  static void EncodeFixed64(char *dst, uint64_t value) {
    uint8_t *const buffer = reinterpret_cast<uint8_t *>(dst);
    std::memcmp(buffer, &value, sizeof(uint64_t));
  }

  static uint8_t DecodeFixed8(const uint8_t *buffer) {
    uint8_t result;
    std::memcpy(&result, buffer, sizeof(uint8_t));
    return result;
  }

  static uint64_t DecodeFixed64(const uint8_t *buffer) {
    uint64_t result;
    std::memcpy(&result, buffer, sizeof(uint64_t));
    return result;
  }

  static void PutFixed8(std::string *dst, uint8_t value) {
    char buf[sizeof(value)];
    EncodeFixed8(buf, value);
    dst->append(buf, sizeof(buf));
  }

  static void PutFixed64(std::string *dst, uint64_t value) {
    char buf[sizeof(value)];
    EncodeFixed64(buf, value);
    dst->append(buf, sizeof(buf));
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

  //
  // RaftLogKey encode
  //
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

  static void DecodeRegionMetaKey(std::vector<uint8_t> key, uint64_t *regionId,
                                  uint8_t *suffix) {
    if ((RegionMetaMinKey.size() + 8 + 1) != (key.size())) {
      *regionID = 0;
      *suffix = 0;
      return;
    }
    if (!((key[0] == RegionMetaMinKey[0]) && (key[1] == RegionMetaMinKey[1]))) {
      *regionID = 0;
      *suffix = 0;
      return;
    }
    *regionID = DecodeFixed64(&key[2]);
    *suffix = key[key.size() - 1];
  }

  static std::string RegionMetaPrefixKey(uint64_t regionId) {
    std::string dst;
    PutFixed8(&dst, kLocalPrefix[0]);
    PutFixed8(&dst, kRegionMetaPrefix[0]);
    PutFixed64(&dst, regionId);
    return dst;
  }

  // kLocalPrefix + kRegionMetaPrefix + regionID + kRegionStateSuffix
  static std::string RegionStateKey(uint64_t regionId) {
    std::string dst;
    PutFixed8(&dst, kLocalPrefix[0]);
    PutFixed8(&dst, kRegionMetaPrefix[0]);
    PutFixed64(&dst, regionId);
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

  static raft_messagepb::RegionLocalState *GetRegionLocalState(
      std::shared_ptr<StorageEngineInterface> db, uint64_t regionId) {
    raft_messagepb::RegionLocalState *regionLocalState;
    GetMessageFromEngine(db, RegionStateKey(regionId), regionLocalState);
    return regionLocalState;
  }

  static EngOpStatus PutMessageToEngine(
      std::shared_ptr<StorageEngineInterface> db, std::string key,
      google::protobuf::Message &msg) {
    std::string val = msg.SerializeAsString();
    return db->PutK(key, val);
  }

  static eraftpb::Entry *GetRaftEntry(
      std::shared_ptr<StorageEngineInterface> db, uint64_t regionId, uint64_t) {
    eraftpb::Entry *entry = new eraftpb::Entry();
    GetMessageFromEngine(db, RaftLogKey(regionId, idx), entry);
    return entry;
  }

  static const uint8_t kRaftInitLogTerm = 5;
  static const uint8_t kRaftInitLogIndex = 5;

  static std::pair<raft_messagepb::RaftLocalState *, storage::EngOpStatus>
  GetRaftLocalState(std::shared_ptr<StorageEngineInterface> raftEngine,
                    uint64_t regionId) {
    raft_messagepb::RaftLocalState *raftLocalState =
        new raft_messagepb::RaftLocalState();
    auto status = GetMessageFromEngine(raftEngine, RaftStateKey(regionId),
                                       raftLocalState);
    return std::pair<raft_messagepb::RaftLocalState *, storage::EngOpStatus>(
        raftLocalState, status);
  }

  static std::pair<raft_messagepb::RaftApplyState *, storage::EngOpStatus>
  GetApplyState(std::shared_ptr<StorageEngineInterface> raftEngine,
                uint64_t regionId) {
    raft_messagepb::RaftApplyState *applyState =
        new raft_messagepb::RaftApplyState();
    auto status =
        GetMessageFromEngine(raftEngine, ApplyStateKey(regionId), applyState);
    return std::pair<raft_messagepb::RaftApplyState *, rocksdb::Status>(
        applyState, status);
  }

  static std::pair<raft_messagepb::RaftApplyState *, bool> InitApplyState(
      std::shared_ptr<StorageEngineInterface> kvEngine,
      std::shared_ptr<metapb::Region> region) {
    auto appyStatePair = GetApplyState(kvEngine, region->id());
    if (appyStatePair.second == storage::EngOpStatus::NOT_FOUND) {
      raft_serverpb::RaftApplyState *applyState =
          new raft_messagepb::RaftApplyState();
      if (region->peers().size() > 0) {
        applyState->set_applied_index(kRaftInitLogIndex);
        applyState->set_index(kRaftInitLogIndex);
        applyState->set_term(kRaftInitLogTerm);
      }
      auto status =
          PutMessageToEngine(kvEngine, ApplyStateKey(region->id()), applyState);
    }
    return std::pair<raft_messagepb::RaftApplyState *, bool>(
        appyStatePair.first, false);
  }

  static std::pair<raft_messagepb::RaftLocalState *, bool> InitRaftLocalState(
      std::shared_ptr<StorageEngineInterface> raftEngine,
      std::shared_ptr<metapb::Region> region) {
    auto raftLocalStatePair = GetRaftLocalState(raftEngine, region->id());
    if (raftLocalStatePair.second == storage::EngOpStatus::NOT_FOUND) {
    }
    return std::pair<raft_messagepb::RaftLocalState *, bool>(
        raftLocalStatePair.first, false);
  }

  static EngOpStatus GetMessageFromEngine(
      std::shared_ptr<StorageEngineInterface> db, std::string key,
      google::protobuf::Message &msg) {
    std::string msgStr;
    auto status = db->GetV(key, msgStr);
    msg.ParseFromString(msgStr);
    return status;
  }
};

}  // namespace network

#endif