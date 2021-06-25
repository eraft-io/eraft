#ifndef ERAFT_KV_STORAGE_H_
#define ERAFT_KV_STORAGE_H_

#include <eraftio/kvrpcpb.pb.h>
#include <vector>
#include <stdint.h>

namespace kvserver
{

struct Modify
{
    Modify(void* data) {
        this->data_ = data;
    }

    void* data_;
};


class Storage
{
public:

    virtual ~Storage() {}

    virtual bool Start() = 0;

    virtual bool Write(kvrpcpb::Context *ctx, std::vector<Modify>) = 0;

    virtual bool Reader(kvrpcpb::Context *ctx) = 0; // TODO: return something

};

} // namespace kvserver

#endif