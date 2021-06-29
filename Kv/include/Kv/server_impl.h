#ifndef ERAFT_KV_SERVER_IMPL_H_
#define ERAFT_KV_SERVER_IMPL_H_

#include <eraftio/tinykvpb.grpc.pb.h>
#include <eraftio/raft_serverpb.pb.h>
#include <eraftio/metapb.pb.h>
#include <grpcpp/grpcpp.h>
#include <iostream>

#include <Kv/storage.h>

using tinykvpb::TinyKv;
using grpc::Status;
using grpc::ServerContext;
using raft_serverpb::Done;

namespace kvserver
{

const std::string DEFAULT_ADDR = "127.0.0.1:12306";

class Server
{

public:

    Server();

    Server(std::string addr);

    bool RunLogic();

    ~Server();

private:

    std::string serverAddress_;

};


class ServerServiceImpl : public TinyKv::Service {

public:

    Status Raft(ServerContext* context, const raft_serverpb::RaftMessage* request, Done* response) override;

    Status RawGet(ServerContext* context, const kvrpcpb::RawGetRequest* request, kvrpcpb::RawGetResponse* response) override;
   
    Status RawPut(ServerContext* context, const kvrpcpb::RawPutRequest* request, kvrpcpb::RawPutResponse* response) override;
    
    Status RawDelete(ServerContext* context, const kvrpcpb::RawDeleteRequest* request, kvrpcpb::RawDeleteResponse* response) override;
    
    Status RawScan(ServerContext* context, const kvrpcpb::RawScanRequest* request, kvrpcpb::RawScanResponse* response) override;

    Status Snapshot(ServerContext* context, const raft_serverpb::SnapshotChunk* request, Done* response) override;

private:

    // Storage* st_;
};

    
} // namespace kvserver

#endif