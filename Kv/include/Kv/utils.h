#ifndef ERAFT_KV_UTIL_H_
#define ERAFT_KV_UTIL_H_

#include <string>
#include <memory>
#include <vector>
#include <stdint.h>
#include <rocksdb/db.h>
#include <google/protobuf/message.h>
#include <eraftio/eraftpb.pb.h>
#include <rocksdb/write_batch.h>
#include <cassert>
#include <eraftio/raft_serverpb.pb.h>

#include <Kv/engines.h>
#include <Logger/Logger.h>
#include <google/protobuf/text_format.h>

namespace kvserver
{

const std::vector<uint8_t> kLocalPrefix = { 0x01 };

const std::vector<uint8_t> kRegionRaftPrefix = { 0x02 };

const std::vector<uint8_t> kRegionMetaPrefix = { 0x03 };

const uint8_t kRegionRaftPrefixLen = 11;

const uint8_t kRegionRaftLogLen = 19;

const uint64_t kInvalidID = 0;

const std::vector<uint8_t> kRaftLogSuffix = { 0x01 };

const std::vector<uint8_t> kRaftStateSuffix = { 0x02 };

const std::vector<uint8_t> kApplyStateSuffix = { 0x03 };

const std::vector<uint8_t> kRegionStateSuffix = { 0x01 };

const std::vector<uint8_t> MinKey = {};

const std::vector<uint8_t> MaxKey = { 255 };

const std::vector<uint8_t> LocalMinKey = { 0x01 };

const std::vector<uint8_t> LocalMaxKey = { 0x02 };

const std::vector<uint8_t> RegionMetaMinKey = { 0x01, 0x03 };

const std::vector<uint8_t> RegionMetaMaxKey = { 0x01, 0x04 };

const std::vector<uint8_t> PrepareBootstrapKey = { 0x01, 0x01 };

const std::vector<uint8_t> StoreIdentKey = { 0x01, 0x02 };


static std::string VecToString(std::vector<uint8_t> in) 
{
    return std::string(in.begin(), in.end());
}

static std::vector<uint8_t> StringToVec(std::string in)
{
    return std::vector<uint8_t>(in.begin(), in.end());
}

static void WriteU8(std::vector< uint8_t >& packet, uint8_t v) 
{
    packet.push_back(v);
}

static void WriteU16(std::vector< uint8_t >& packet, uint16_t v) 
{
    packet.push_back(static_cast< uint8_t >(v));
    packet.push_back(static_cast< uint8_t >(v >> 8));
}

static void WriteU24(std::vector< uint8_t >& packet, uint32_t v) 
{
    packet.push_back(static_cast< uint8_t >(v));
    packet.push_back(static_cast< uint8_t >(v >> 8));
    packet.push_back(static_cast< uint8_t >(v >> 16));          
}

static void WriteU32(std::vector< uint8_t >& packet, uint32_t v) 
{
    packet.push_back(static_cast< uint8_t >(v));
    packet.push_back(static_cast< uint8_t >(v >> 8));
    packet.push_back(static_cast< uint8_t >(v >> 16));     
    packet.push_back(static_cast< uint8_t >(v >> 24));              
}

static void WriteU64(std::vector< uint8_t >& packet, uint64_t v) 
{
    packet.push_back(static_cast< uint8_t >(v));
    packet.push_back(static_cast< uint8_t >(v >> 8));
    packet.push_back(static_cast< uint8_t >(v >> 16));     
    packet.push_back(static_cast< uint8_t >(v >> 24));
    packet.push_back(static_cast< uint8_t >(v >> 32));
    packet.push_back(static_cast< uint8_t >(v >> 40));     
    packet.push_back(static_cast< uint8_t >(v >> 48));     
    packet.push_back(static_cast< uint8_t >(v >> 56));
}

static uint8_t ReadU8(std::vector< uint8_t>::iterator& it) 
{
    // packet end check, TODO
    uint8_t v;
    v = *it;
    return v;
}

static uint16_t ReadU16(std::vector< uint8_t>::iterator& it) {
    // packet end check, TODO
    uint16_t v;
    v = static_cast< uint16_t >(*it) |
        static_cast< uint16_t >(*(it + 1) << 8);
    return v;
}

static uint16_t ReadU24(std::vector< uint8_t>::iterator& it) 
{
    uint32_t v;
    v = static_cast< uint32_t >(*it) |
        static_cast< uint32_t >(*(it + 1) << 8) |
        static_cast< uint32_t >(*(it + 2) << 16);
    return v;
}

static uint32_t ReadU32(std::vector< uint8_t>::iterator& it) 
{
    // packet end check, TODO
    uint32_t v;
    v = static_cast< uint32_t >(*it) |
        static_cast< uint32_t >(*(it + 1) << 8) |
        static_cast< uint32_t >(*(it + 2) << 16) |
        static_cast< uint32_t >(*(it + 3) << 24);
    return v;
}

static uint64_t ReadU64(std::vector< uint8_t>::iterator& it) 
{
    uint64_t v;
    v = static_cast< uint64_t >(*it) |
        static_cast< uint64_t >(*(it + 1) << 8) |
        static_cast< uint64_t >(*(it + 2) << 16) |
        static_cast< uint64_t >(*(it + 3) << 24) |
        static_cast< uint64_t >(*(it + 4) << 32) |
        static_cast< uint64_t >(*(it + 5) << 40) |
        static_cast< uint64_t >(*(it + 6) << 48) |
        static_cast< uint64_t >(*(it + 7) << 56);
    return v;         
}

//
//  CF defines
//
const std::string CfDefault = "default";
const std::string CfWrite = "write";
const std::string CfLock = "lock";

static const std::vector<std::string> CFs = { CfDefault, CfWrite, CfLock };

static std::string KeyWithCF(std::string cf, std::string key) 
{
    return cf + "_"  + key ;
}

static std::string GetCF(rocksdb::DB* db, std::string cf, std::string key) 
{
    std::string res;
    rocksdb::Status s = db->Get(rocksdb::ReadOptions(), KeyWithCF(cf, key), &res);
    if(s.ok()) {
        return res;
    } else {
        return "";
    }
}

static bool PutCF(std::unique_ptr<rocksdb::DB> db, std::string cf, std::string key, std::string val)
{
    return (db->Put(rocksdb::WriteOptions(), KeyWithCF(cf, key), val)).ok();
}

static rocksdb::Status GetMeta(rocksdb::DB* db, std::string key, google::protobuf::Message* msg)
{
    std::string res;
    rocksdb::Status s = db->Get(rocksdb::ReadOptions(), key, &res);
    msg->ParseFromString(res);
    return s;
}

static bool PutMeta(rocksdb::DB* db, std::string key, google::protobuf::Message& msg)
{
    std::string val;
    google::protobuf::TextFormat::PrintToString(msg, &val);
    Logger::GetInstance()->INFO("put key: " + key + " val: " + val + " to db");
    auto status = db->Put(rocksdb::WriteOptions(), key, val);

    return status.ok();
}

static bool SetMeta(rocksdb::WriteBatch* batch, std::string key, google::protobuf::Message& msg)
{
    std::string val;
    google::protobuf::TextFormat::PrintToString(msg, &val);
    Logger::GetInstance()->INFO("put key: " + key + " val: " + val + " to db");
    return batch->Put(key, val).ok();
}

//
// RegionPrefix: kLocalPrefix + kRegionRaftPrefix + regionID + suffix
//
static std::vector<uint8_t> MakeRegionPrefix(uint64_t regionID, std::vector<uint8_t> suffix) 
{
    std::vector<uint8_t> packet;
    packet.insert(packet.begin(), kLocalPrefix.begin(), kLocalPrefix.end());
    packet.insert(packet.end(), kRegionRaftPrefix.begin(), kRegionRaftPrefix.end());
    WriteU64(packet, regionID);
    packet.insert(packet.end(), suffix.begin(), suffix.end());
    return packet;
}

//
// RegionKey: kLocalPrefix + kRegionRaftPrefix + regionID + suffix + subID
//

static std::vector<uint8_t> MakeRegionKey(uint64_t regionID, std::vector<uint8_t> suffix, uint64_t subID)
{
    std::vector<uint8_t> packet;
    packet.insert(packet.begin(), kLocalPrefix.begin(), kLocalPrefix.end());
    packet.insert(packet.end(), kRegionRaftPrefix.begin(), kRegionRaftPrefix.end());
    WriteU64(packet, regionID);
    packet.insert(packet.end(), suffix.begin(), suffix.end());
    WriteU64(packet, subID);
    return packet;
}

//
//  RegionRaftPrefixKey: kLocalPrefix + kRegionRaftPrefix + regionID
//

static std::vector<uint8_t> RegionRaftPrefixKey(uint64_t regionID)
{
    std::vector<uint8_t> packet;
    packet.insert(packet.begin(), kLocalPrefix.begin(), kLocalPrefix.end());
    packet.insert(packet.end(), kRegionRaftPrefix.begin(), kRegionRaftPrefix.end());
    WriteU64(packet, regionID);
    return packet;
}

static std::vector<uint8_t> RaftLogKey(uint64_t regionID, uint64_t index)
{
    return MakeRegionKey(regionID, kRaftLogSuffix, index);
}

static std::vector<uint8_t> RaftStateKey(uint64_t regionID)
{
    return MakeRegionPrefix(regionID, kRaftStateSuffix);
}

static std::vector<uint8_t> ApplyStateKey(uint64_t regionID)
{
    return MakeRegionPrefix(regionID, kApplyStateSuffix);
}

static bool IsRaftStateKey(std::vector<uint8_t> key)
{
    assert(key.size() >= 2);
    return (key.size() == 11 && key[0] == kLocalPrefix[0] && key[1] == kRegionRaftPrefix[0]);
}

static void DecodeRegionMetaKey(std::vector<uint8_t> key, uint64_t* regionID, uint8_t* suffix)
{
    if((RegionMetaMinKey.size() + 8 + 1) != (key.size()))
    {
        // TODO: log invalid region meta key length for key
        *regionID = 0; *suffix = 0;
        return;
    }
    if( !((key[0] == RegionMetaMinKey[0]) && (key[1] == RegionMetaMinKey[1])) ) {
        // TODO: invalid region meta key prefix for key
        *regionID = 0; *suffix = 0;
        return;
    }
    std::vector<uint8_t>::iterator it = key.begin() + RegionMetaMinKey.size();
    *regionID = ReadU64(it);
    *suffix = key[key.size()-1];
}

static std::vector<uint8_t> RegionMetaPrefixKey(uint64_t regionID)
{
    std::vector<uint8_t> packet;
    packet.insert(packet.begin(), kLocalPrefix.begin(), kLocalPrefix.end());
    packet.insert(packet.end(), kRegionMetaPrefix.begin(), kRegionMetaPrefix.end());
    WriteU64(packet, regionID);
    return packet;
}

// kLocalPrefix + kRegionMetaPrefix + regionID + kRegionStateSuffix
static std::vector<uint8_t> RegionStateKey(uint64_t regionID)
{
    std::vector<uint8_t> packet;
    packet.insert(packet.begin(), kLocalPrefix.begin(), kLocalPrefix.end());
    packet.insert(packet.end(), kRegionMetaPrefix.begin(), kRegionMetaPrefix.end());
    WriteU64(packet, regionID);
    packet.insert(packet.end(), kRegionStateSuffix.begin(), kRegionStateSuffix.end());
    return packet;
}

/// RaftLogIndex gets the log index from raft log key generated by RaftLogKey.
static uint64_t RaftLogIndex(std::vector<uint8_t> key)
{
    if(key.size() != kRegionRaftLogLen)
    {
        // log key is not a valid raft log key
        return 0;
    }
    std::vector<uint8_t>::iterator it = key.begin() + (kRegionRaftLogLen - 8);
    return ReadU64(it);
}

static raft_serverpb::RegionLocalState* GetRegionLocalState(rocksdb::DB* db, uint64_t regionId)
{
    raft_serverpb::RegionLocalState* regionLocalState;
    GetMeta(db, std::string(RegionStateKey(regionId).begin(), RegionStateKey(regionId).end()), regionLocalState);
    return regionLocalState;
}

static std::pair<raft_serverpb::RaftLocalState*, rocksdb::Status> GetRaftLocalState(rocksdb::DB* db, uint64_t regionId)
{
    raft_serverpb::RaftLocalState* raftLocalState;
    rocksdb::Status s = GetMeta(db, std::string(RaftStateKey(regionId).begin(), RaftStateKey(regionId).end()), raftLocalState);
    return std::pair<raft_serverpb::RaftLocalState*, rocksdb::Status> (raftLocalState, s);
}

static std::pair<raft_serverpb::RaftApplyState*, rocksdb::Status> GetApplyState(rocksdb::DB* db, uint64_t regionId)
{
    raft_serverpb::RaftApplyState* applyState;
    rocksdb::Status s = GetMeta(db, std::string(ApplyStateKey(regionId).begin(), ApplyStateKey(regionId).end()), applyState);
    return std::pair<raft_serverpb::RaftApplyState*, rocksdb::Status> (applyState, s);   
}

static eraftpb::Entry* GetRaftEntry(rocksdb::DB* db, uint64_t regionId, uint64_t idx)
{
    eraftpb::Entry* entry;
    GetMeta(db, std::string(RaftLogKey(regionId, idx).begin(), RaftLogKey(regionId, idx).end()), entry);
    return entry;
}

const uint8_t kRaftInitLogTerm = 5;
const uint8_t kRaftInitLogIndex = 5;

//
//  This function init raft local state to db
// 
static std::pair<raft_serverpb::RaftLocalState*, bool> InitRaftLocalState(rocksdb::DB* raftEngine, std::shared_ptr<metapb::Region> region)
{
    auto lst = GetRaftLocalState(raftEngine, region->id());
    if(lst.second.IsNotFound()) 
    {
        raft_serverpb::RaftLocalState raftState;
        raftState.set_allocated_hard_state(new eraftpb::HardState);
        // new split region
        if (region->peers().size() > 0)
        {
            raftState.set_last_index(kRaftInitLogIndex);
            raftState.set_last_term(kRaftInitLogTerm);
            raftState.mutable_hard_state()->set_term(kRaftInitLogTerm);
            raftState.mutable_hard_state()->set_commit(kRaftInitLogIndex);
            if(!PutMeta(raftEngine, std::string(RaftStateKey(region->id()).begin(), RaftStateKey(region->id()).end()), raftState))
            {
                return std::pair<raft_serverpb::RaftLocalState*, bool>(&raftState, false);
            }
        }
    }
    return std::pair<raft_serverpb::RaftLocalState*, bool>(lst.first, true);
}

static std::pair<raft_serverpb::RaftApplyState*, bool> InitApplyState(rocksdb::DB* kvEngine, std::shared_ptr<metapb::Region> region)
{
    auto ast = GetApplyState(kvEngine, region->id());
    if(ast.second.IsNotFound()) 
    {
        raft_serverpb::RaftApplyState applyState;
        applyState.set_allocated_truncated_state(new raft_serverpb::RaftTruncatedState);
        if(region->peers().size() > 0)
        {
            applyState.set_applied_index(kRaftInitLogIndex);
            applyState.mutable_truncated_state()->set_index(kRaftInitLogIndex);
            applyState.mutable_truncated_state()->set_term(kRaftInitLogTerm);
        }
        if(!PutMeta(kvEngine, std::string(ApplyStateKey(region->id()).begin(), ApplyStateKey(region->id()).end()), applyState))
        {
            return std::pair<raft_serverpb::RaftApplyState*, bool>(&applyState, false);
        }
    }
    return std::pair<raft_serverpb::RaftApplyState*, bool>(ast.first, false);
}

//
// write region state to kv write batch
//
static void WriteRegionState(std::shared_ptr<rocksdb::WriteBatch> kvWB, std::shared_ptr<metapb::Region> region, const raft_serverpb::PeerState& state)
{
    raft_serverpb::RegionLocalState regionState;
    regionState.set_state(state);
    regionState.set_allocated_region(region.get());
    SetMeta(kvWB.get(), std::string(RegionStateKey(region->id()).begin(), RegionStateKey(region->id()).end()), regionState);
}



static bool DeleteCF(std::unique_ptr<rocksdb::DB> db, std::string cf, std::string key)
{   
    return (db->Delete(rocksdb::WriteOptions(), KeyWithCF(cf, key))).ok();
}

static void DeleteRangeCF(std::shared_ptr<rocksdb::DB> db, std::shared_ptr<rocksdb::WriteBatch> batch, 
                          std::string cf, std::string startKey, std::string endKey)
{
    batch->DeleteRange(KeyWithCF(cf, startKey), KeyWithCF(cf, endKey));
}

static bool DeleteRange(std::shared_ptr<rocksdb::DB> db, std::string startKey, std::string endKey)
{
    std::shared_ptr<rocksdb::WriteBatch> batch;
    for(auto cf: CFs) {
        DeleteRangeCF(db, batch, cf, startKey, endKey);
    }
    return db->Write(rocksdb::WriteOptions(), & *batch).ok();
}

static rocksdb::Iterator* NewCFIterator(rocksdb::DB* db, std::string cf)
{
    rocksdb::ReadOptions read_options;
    read_options.auto_prefix_mode = true;
    auto iter = db->NewIterator(read_options);
    iter->Seek(cf + "_");
    return iter;
}

static bool ExceedEndKey(std::string current, std::string endKey)
{
    if( endKey.size() == 0 )
    {
        return false;
    }
    return current.compare(endKey) >= 0;
}


static const uint64_t kRaftInvalidIndex = 0;

static bool IsInitialMsg(eraftpb::Message& msg)
{
    return msg.msg_type() == eraftpb::MsgRequestVote || (msg.msg_type() == eraftpb::MsgHeartbeat && msg.commit() == kRaftInvalidIndex);
}

static eraftpb::ConfState ConfStateFromRegion(std::shared_ptr<metapb::Region> region)
{
    eraftpb::ConfState confState;
    for(auto p: region->peers())
    {
        confState.add_nodes(p.id());
    }
    return confState;
}

static bool DoClearMeta(std::shared_ptr<Engines> engines, std::shared_ptr<rocksdb::WriteBatch> kvWB,
                        std::shared_ptr<rocksdb::WriteBatch> raftWB, uint64_t regionID, uint64_t lastIndex)
{
    // TODO: stat cost time
    kvWB->Delete(std::string(RegionStateKey(regionID).begin(), RegionStateKey(regionID).end()));
    kvWB->Delete(std::string(ApplyStateKey(regionID).begin(), ApplyStateKey(regionID).end()));

    uint64_t firstIndex = lastIndex + 1;
    std::string beginLogKey(RaftLogKey(regionID, 0).begin(), RaftLogKey(regionID, 0).end());
    std::string endLogKey(RaftLogKey(regionID, firstIndex).begin(), RaftLogKey(regionID, firstIndex).end());

    auto iter = engines->raftDB_->NewIterator(rocksdb::ReadOptions());
    iter->Seek(beginLogKey);
    if(iter->Valid() && iter->key().ToString().compare(endLogKey) < 0) 
    {
       uint64_t logIdx = RaftLogIndex(std::vector<uint8_t>{iter->key().ToString().begin(), iter->key().ToString().end()});
       firstIndex = logIdx;
    }

    for(uint64_t i = firstIndex; i <= lastIndex; i ++)
    {
        raftWB->Delete(std::string(RaftLogKey(regionID, i).begin(), RaftLogKey(regionID, i).end()));
    }

    raftWB->Delete(std::string(RaftStateKey(regionID).begin(), RaftStateKey(regionID).end()));

    return true;
}

} // namespace kvserver

#endif
