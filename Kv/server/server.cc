#include <Kv/server.h>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <iostream>

namespace kvserver
{

Server::Server() {
    this->serverAddress_ = DEFAULT_ADDR;
}

Server::Server(std::string addr) {
    this->serverAddress_ = addr;
}

Status Server::Raft(ServerContext* context, const raft_serverpb::RaftMessage* request, Done* response) {
    std::cout << "start_key: " << request->start_key() << std::endl;
    return Status::OK;
}

Status Server::RawGet(ServerContext* context, const kvrpcpb::RawGetRequest* request, kvrpcpb::RawGetResponse* response) {
    
}

Status Server::RawPut(ServerContext* context, const kvrpcpb::RawPutRequest* request, kvrpcpb::RawPutResponse* response) {

}

Status Server::RawDelete(ServerContext* context, const kvrpcpb::RawDeleteRequest* request, kvrpcpb::RawDeleteResponse* response) {

}

Status Server::RawScan(ServerContext* context, const kvrpcpb::RawScanRequest* request, kvrpcpb::RawScanResponse* response) {

}

Status Server::Snapshot(ServerContext* context, const raft_serverpb::SnapshotChunk* request, Done* response) {

}

bool Server::RunLogic() {
    Server service;
    grpc::EnableDefaultHealthCheckService(true);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(this->serverAddress_, grpc::InsecureServerCredentials());

    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    // TODO: print log (server listening on server address)
    std::cout << "server listening on: " << this->serverAddress_ << std::endl;
    server->Wait();
}

Server::~Server() {

}

} // namespace kvserver
