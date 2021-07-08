#ifndef ERAFT_KV_UTIL_H_
#define ERAFT_KV_UTIL_H_

#include <string>
#include <memory>
#include <vector>
#include <stdint.h>
#include <leveldb/db.h>
#include <google/protobuf/message.h>

namespace kvserver
{

static std::string KeyWithCF(std::string cf, std::string key) 
{
    return cf + "_"  + key ;
}

static std::string GetCF(std::unique_ptr<leveldb::DB> db, std::string cf, std::string key) 
{
    std::string res;
    leveldb::Status s = db->Get(leveldb::ReadOptions(), KeyWithCF(cf, key), &res);
    if(s.ok()) {
        return res;
    } else {
        return "";
    }
}

static bool PutCF(std::unique_ptr<leveldb::DB> db, std::string cf, std::string key, std::string val)
{
    return (db->Put(leveldb::WriteOptions(), KeyWithCF(cf, key), val)).ok();
}

static void GetMeta(std::unique_ptr<leveldb::DB> db, std::string key, google::protobuf::Message* msg)
{
    std::string res;
    leveldb::Status s = db->Get(leveldb::ReadOptions(), key, &res);
    msg->ParseFromString(res);
}

static bool PutMeta(std::unique_ptr<leveldb::DB> db, std::string key, google::protobuf::Message& msg)
{
    std::string val;
    msg.SerializeToString(&val);
    return (db->Put(leveldb::WriteOptions(), key, val)).ok();
}



static void WriteU8(std::vector< uint8_t >& packet, uint8_t v) {
    packet.push_back(v);
}

static void WriteU16(std::vector< uint8_t >& packet, uint16_t v) {
    packet.push_back(static_cast< uint8_t >(v));
    packet.push_back(static_cast< uint8_t >(v >> 8));
}

static void WriteU24(std::vector< uint8_t >& packet, uint32_t v) {
    packet.push_back(static_cast< uint8_t >(v));
    packet.push_back(static_cast< uint8_t >(v >> 8));
    packet.push_back(static_cast< uint8_t >(v >> 16));          
}

static void WriteU32(std::vector< uint8_t >& packet, uint32_t v) {
    packet.push_back(static_cast< uint8_t >(v));
    packet.push_back(static_cast< uint8_t >(v >> 8));
    packet.push_back(static_cast< uint8_t >(v >> 16));     
    packet.push_back(static_cast< uint8_t >(v >> 24));              
}

static void WriteU64(std::vector< uint8_t >& packet, uint64_t v) {
    packet.push_back(static_cast< uint8_t >(v));
    packet.push_back(static_cast< uint8_t >(v >> 8));
    packet.push_back(static_cast< uint8_t >(v >> 16));     
    packet.push_back(static_cast< uint8_t >(v >> 24));
    packet.push_back(static_cast< uint8_t >(v >> 32));
    packet.push_back(static_cast< uint8_t >(v >> 40));     
    packet.push_back(static_cast< uint8_t >(v >> 48));     
    packet.push_back(static_cast< uint8_t >(v >> 56));
}

static uint8_t ReadU8(const std::vector< uint8_t>::iterator& it) {
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

static uint16_t ReadU24(const std::vector< uint8_t>::iterator& it) {
    uint32_t v;
    v = static_cast< uint32_t >(*it) |
        static_cast< uint32_t >(*(it + 1) << 8) |
        static_cast< uint32_t >(*(it + 2) << 16);
    return v;
}

static uint32_t ReadU32(const std::vector< uint8_t>::iterator& it) {
    // packet end check, TODO
    uint32_t v;
    v = static_cast< uint32_t >(*it) |
        static_cast< uint32_t >(*(it + 1) << 8) |
        static_cast< uint32_t >(*(it + 2) << 16) |
        static_cast< uint32_t >(*(it + 3) << 24);
    return v;
}

static uint64_t ReadU64(const std::vector< uint8_t>::iterator& it) {
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

} // namespace kvserver

#endif
