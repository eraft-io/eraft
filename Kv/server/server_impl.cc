#include <Kv/server_impl.h>

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

Status ServerServiceImpl::Raft(ServerContext* context, const raft_serverpb::RaftMessage* request, Done* response) {
    std::cout << "start_key: " << request->start_key() << std::endl;
    
    return Status::OK;
}

Status ServerServiceImpl::RawGet(ServerContext* context, const kvrpcpb::RawGetRequest* request, kvrpcpb::RawGetResponse* response) {
    
}

Status ServerServiceImpl::RawPut(ServerContext* context, const kvrpcpb::RawPutRequest* request, kvrpcpb::RawPutResponse* response) {

}

Status ServerServiceImpl::RawDelete(ServerContext* context, const kvrpcpb::RawDeleteRequest* request, kvrpcpb::RawDeleteResponse* response) {

}

Status ServerServiceImpl::RawScan(ServerContext* context, const kvrpcpb::RawScanRequest* request, kvrpcpb::RawScanResponse* response) {

}

Status ServerServiceImpl::Snapshot(ServerContext* context, const raft_serverpb::SnapshotChunk* request, Done* response) {

}

bool Server::RunLogic() {

    ServerServiceImpl service;
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
