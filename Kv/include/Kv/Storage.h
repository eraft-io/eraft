#ifndef ERAFT_KV_STORAGE_H_
#define ERAFT_KV_STORAGE_H_

#include <eraftio/kvrpcpb.pb.h>
#include <vector>
#include <stdint.h>
#include <string>
#include <leveldb/db.h>

namespace kvserver
{


/// 对 kv 服务器操作的一些抽象

enum class OpType 
{
    Get,
    Put,
    Delete,
};

struct Put
{
    std::string key;
    std::string value;
    std::string cf;
};

struct Delete
{
    std::string key;
    std::string cf;
};

struct Modify
{
    Modify(void* data, OpType ot) {
        this->data_ = data;
        this->ot_ = ot;
    }

    std::string Key() {
        switch (this->ot_)
        {
        case OpType::Put:
            {
                struct Put* pt = (struct Put*)this->data_;
                return pt->key;
            }
        case OpType::Delete:
            {
                struct Delete* dt = (struct Delete*)this->data_;
                return dt->key;
            }
        default:
            break;
        }
        return "";
    }

    std::string Value() {
        if(ot_ == OpType::Put) {
            struct Put* pt = (struct Put*)this->data_;
            return pt->value;
        }
        return "";
    }

    std::string Cf() {
        switch (this->ot_)
        {
        case OpType::Put:
            {
                struct Put* pt = (struct Put*)this->data_;
                return pt->cf;
            }
        case OpType::Delete:
            {
                struct Delete* dt = (struct Delete*)this->data_;
                return dt->key;
            }
        default:
            break;
        }
        return "";
    }
    
    OpType ot_;

    void* data_;
};

class StorageReader
{
public:
    
    virtual  ~StorageReader();

    virtual std::vector<uint8_t> GetCF(std::string cf, std::vector<uint8_t> key) = 0;

    virtual leveldb::Iterator* IterCF(std::uint8_t cf) = 0;

    virtual void Close() = 0;
};

class Storage
{
public:

    virtual ~Storage() {}

    virtual bool Start() = 0;

    virtual bool Write(kvrpcpb::Context *ctx, std::vector<Modify>) = 0;

    virtual StorageReader* Reader(kvrpcpb::Context *ctx) = 0; // TODO: return something

};



} // namespace kvserver

#endif