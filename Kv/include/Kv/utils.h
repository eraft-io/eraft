#ifndef ERAFT_KV_UTIL_H_
#define ERAFT_KV_UTIL_H_

#include <string>
#include <memory>
#include <vector>
#include <stdint.h>
#include <rocksdb/db.h>
#include <google/protobuf/message.h>
#include <Kv/meta.h>
#include <eraftio/eraftpb.pb.h>
#include <rocksdb/write_batch.h>

namespace kvserver
{


static std::string KeyWithCF(std::string cf, std::string key) 
{
    return cf + "_"  + key ;
}

static std::string GetCF(std::unique_ptr<rocksdb::DB> db, std::string cf, std::string key) 
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

static rocksdb::Status GetMeta(std::shared_ptr<rocksdb::DB> db, std::string key, google::protobuf::Message* msg)
{
    std::string res;
    rocksdb::Status s = db->Get(rocksdb::ReadOptions(), key, &res);
    msg->ParseFromString(res);
    return s;
}

static bool PutMeta(std::shared_ptr<rocksdb::DB> db, std::string key, google::protobuf::Message& msg)
{
    std::string val;
    msg.SerializeToString(&val);
    return (db->Put(rocksdb::WriteOptions(), key, val)).ok();
}

static bool SetMeta(std::shared_ptr<rocksdb::WriteBatch> batch, std::string key, google::protobuf::Message& msg)
{
    std::string val;
    msg.SerializeToString(&val);
    batch->Put(key, val).ok();
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

static rocksdb::Iterator* NewCFIterator(std::shared_ptr<rocksdb::DB> db, std::string cf)
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

static uint8_t ReadU8(const std::vector< uint8_t>::iterator& it) 
{
    // packet end check, TODO
    uint8_t v;
    v = *it;
    return v;
}

static uint16_t ReadU16(const std::vector< uint8_t>::iterator& it) {
    // packet end check, TODO
    uint16_t v;
    v = static_cast< uint16_t >(*it) |
        static_cast< uint16_t >(*(it + 1) << 8);
    return v;
}

static uint16_t ReadU24(const std::vector< uint8_t>::iterator& it) 
{
    uint32_t v;
    v = static_cast< uint32_t >(*it) |
        static_cast< uint32_t >(*(it + 1) << 8) |
        static_cast< uint32_t >(*(it + 2) << 16);
    return v;
}

static uint32_t ReadU32(const std::vector< uint8_t>::iterator& it) 
{
    // packet end check, TODO
    uint32_t v;
    v = static_cast< uint32_t >(*it) |
        static_cast< uint32_t >(*(it + 1) << 8) |
        static_cast< uint32_t >(*(it + 2) << 16) |
        static_cast< uint32_t >(*(it + 3) << 24);
    return v;
}

static uint64_t ReadU64(const std::vector< uint8_t>::iterator& it) 
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
