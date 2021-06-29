#ifndef ERAFT_KV_ENGINES_H_
#define ERAFT_KV_ENGINES_H_

#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <memory>
#include <cassert>

namespace kvserver
{

struct Engines
{
    Engines(std::string kvPath, std::string raftPath);

    ~Engines();

    leveldb::DB* kvDB_;

    std::string kvPath_;

    leveldb::DB* raftDB_;

    std::string raftPath_;

    bool WriteKV(leveldb::WriteBatch& batch);

    bool WriteRaft(leveldb::WriteBatch& batch);

    bool Close();

    bool Destory();

};

} // namespace kvserver


#endif