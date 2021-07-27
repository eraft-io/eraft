#ifndef ERAFT_KV_RAFT_SERVER_H_
#define ERAFT_KV_RAFT_SERVER_H_

#include <eraftio/raft_cmdpb.pb.h>
#include <eraftio/kvrpcpb.pb.h>
#include <eraftio/tinykvpb.grpc.pb.h>
#include <eraftio/raft_serverpb.pb.h>

#include <Kv/engines.h>
#include <Kv/config.h>
#include <Kv/router.h>
#include <Kv/storage.h>
#include <Kv/node.h>
#include <Kv/raft_store.h>

#include <memory>

namespace kvserver
{

class RegionReader : public StorageReader
{
public:

    RegionReader(std::shared_ptr<Engines> engs, metapb::Region region);

    ~RegionReader();

    std::string GetFromCF(std::string cf, std::string key);

    rocksdb::Iterator* IterCF(std::string cf);

    void Close();

private:

    std::shared_ptr<Engines> engs_;

    metapb::Region region_;

};

    
class RaftStorage : public Storage
{

public:
    
    RaftStorage(std::shared_ptr<Config> conf);

    ~RaftStorage();

    bool CheckResponse(raft_cmdpb::RaftCmdResponse* resp, int reqCount);

    bool Write(const kvrpcpb::Context& ctx, const kvrpcpb::RawPutRequest* put);

    StorageReader* Reader(const kvrpcpb::Context& ctx); 

    bool Raft(const raft_serverpb::RaftMessage* msg);

    bool SnapShot(raft_serverpb::RaftSnapshotData* snap);

    bool Start();

    bool Stop();
    
    std::shared_ptr<Engines> engs_;

private:


    std::shared_ptr<Config> conf_;

    std::shared_ptr<Node> node_;

    std::shared_ptr<RaftRouter> raftRouter_;

    std::shared_ptr<RaftStore> raftSystem_;
    
    RegionReader* regionReader_;
};



} // namespace kvserver


#endif // ERAFT_KV_RAFT_SERVER_H_