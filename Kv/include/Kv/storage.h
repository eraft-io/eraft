#ifndef ERAFT_KV_STORAGE_H_
#define ERAFT_KV_STORAGE_H_

#include <eraftio/kvrpcpb.pb.h>
#include <vector>
#include <stdint.h>
#include <string>
#include <rocksdb/db.h>

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
    Put(std::string key, std::string value, std::string cf)
    {
        this->key_ = key;
        this->value_ = value;
        this->cf_ = cf;
    }

    std::string key_;
    std::string value_;
    std::string cf_;
};

struct Delete
{
    Delete(std::string key, std::string cf)
    {
        this->key = key;
        this->cf = cf;
    }

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
                return pt->key_;
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
            return pt->value_;
        }
        return "";
    }

    std::string Cf() {
        switch (this->ot_)
        {
        case OpType::Put:
            {
                struct Put* pt = (struct Put*)this->data_;
                return pt->cf_;
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
    
    virtual ~StorageReader() {};

    virtual std::string GetFromCF(std::string cf, std::string key) = 0;

    virtual rocksdb::Iterator* IterCF(std::string cf) = 0;

    virtual void Close() = 0;
};

class Storage
{
public:

    virtual ~Storage() {}

    virtual bool Start() = 0;

    virtual bool Write(const kvrpcpb::Context& ctx, const kvrpcpb::RawPutRequest* put) = 0;

    virtual StorageReader* Reader(const kvrpcpb::Context& ctx) = 0; // TODO: return something

};



} // namespace kvserver

#endif