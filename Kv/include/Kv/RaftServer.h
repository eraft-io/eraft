#ifndef ERAFT_KV_RAFT_SERVER_H_
#define ERAFT_KV_RAFT_SERVER_H_

#include <Kv/Engines.h>
#include <Kv/Config.h>
#include <Kv/Node.h>
#include <Kv/Router.h>
#include <Kv/Storage.h>

#include <eraftio/raft_cmdpb.pb.h>
#include <eraftio/kvrpcpb.pb.h>
#include <eraftio/tinykvpb.grpc.pb.h>
#include <eraftio/raft_serverpb.pb.h>

namespace kvserver
{
    
class RaftStorage
{

public:
    
    RaftStorage(Config* conf);

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

    Config* conf_;

    Node* node_;

    RaftstoreRouter* raftRouter_;

    RaftStore* raftSystem_;
    
};


} // namespace kvserver



#endif