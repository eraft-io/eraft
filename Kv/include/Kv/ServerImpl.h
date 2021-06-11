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

    Status Raft(ServerContext* context, const raft_serverpb::RaftMessage* request, Done* response) override {
        // metapb::Peer peer = request->from_peer();
        // request->to_peer();
        std::cout << "start_key: " << request->start_key() << std::endl;
        return Status::OK;
    }

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

