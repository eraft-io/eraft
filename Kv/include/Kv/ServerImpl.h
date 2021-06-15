#include <eraftio/tinykvpb.grpc.pb.h>
#include <eraftio/metapb.pb.h>
#include <grpcpp/grpcpp.h>
#include <iostream>

using tinykvpb::TinyKv;
using grpc::Status;
using grpc::ServerContext;
using raft_serverpb::Done;

namespace kvserver
{

const std::string DEFAULT_ADDR = "127.0.0.1:12306";

class ServerServiceImpl : public TinyKv::Service {

    Status Raft(ServerContext* context, const raft_serverpb::RaftMessage* request, Done* response) override;

    Status RawGet(ServerContext* context, const kvrpcpb::RawGetRequest* request, kvrpcpb::RawGetResponse* response) override;
   
    Status RawPut(ServerContext* context, const kvrpcpb::RawPutRequest* request, kvrpcpb::RawPutResponse* response) override;
    
    Status RawDelete(ServerContext* context, const kvrpcpb::RawDeleteRequest* request, kvrpcpb::RawDeleteResponse* response) override;
    
    Status RawScan(ServerContext* context, const kvrpcpb::RawScanRequest* request, kvrpcpb::RawScanResponse* response) override;

};

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

    
} // namespace kvserver

