#ifndef ERAFT_KV_ENGINES_H_
#define ERAFT_KV_ENGINES_H_

#include <memory>
#include <cassert>

#include <rocksdb/db.h>
#include <rocksdb/write_batch.h>

namespace kvserver
{

struct Engines
{
    Engines(std::string kvPath, std::string raftPath);

    ~Engines();

    rocksdb::DB* kvDB_;

    std::string kvPath_;

    rocksdb::DB* raftDB_;

    std::string raftPath_;

    bool WriteKV(rocksdb::WriteBatch& batch);

    bool WriteRaft(rocksdb::WriteBatch& batch);

    bool Close();

    bool Destory();

};

} // namespace kvserver


#endif