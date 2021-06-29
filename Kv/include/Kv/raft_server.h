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
    
class RaftStorage : Storage
{

public:
    
    RaftStorage(std::shared_ptr<Config> conf);

    ~RaftStorage();

    bool CheckResponse(raft_cmdpb::RaftCmdResponse* resp, int reqCount);

    bool Write(kvrpcpb::Context* ctx, std::vector<Modify> batch);

    StorageReader* Reader(kvrpcpb::Context* ctx); 

    bool Raft(raft_serverpb::RaftMessage* msg);

    bool SnapShot(raft_serverpb::RaftSnapshotData* snap);

    bool Start();

    bool Stop();

private:

    Engines* engs_;

    std::shared_ptr<Config> conf_;

    Node* node_;

    std::shared_ptr<RaftstoreRouter> raftRouter_;

    std::shared_ptr<RaftStore> raftSystem_;
    
};


} // namespace kvserver


#endif // ERAFT_KV_RAFT_SERVER_H_