#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <iostream>

#include <Kv/server.h>
#include <Kv/raft_server.h>
#include <Logger/Logger.h>

namespace kvserver
{

Server::Server() {
    this->serverAddress_ = DEFAULT_ADDR;
}

Server::Server(std::string addr, RaftStorage* st) {
    this->serverAddress_ = addr;
    this->st_ = st;
}

Status Server::Raft(ServerContext* context, const raft_serverpb::RaftMessage* request, Done* response) 
{
    if(this->st_->Raft(request))
    {
        return Status::OK;
    }
    else
    {
        return Status::CANCELLED;
    }
    return Status::OK;
}

Status Server::RawGet(ServerContext* context, const kvrpcpb::RawGetRequest* request, kvrpcpb::RawGetResponse* response) 
{
    auto reader = this->st_->Reader(request->context());
    auto val = reader->GetFromCF(request->cf(), request->key());
    response->set_value(val);
    return Status::OK;
}

Status Server::RawPut(ServerContext* context, const kvrpcpb::RawPutRequest* request, kvrpcpb::RawPutResponse* response) 
{
    Logger::GetInstance()->DEBUG_NEW("handle raw put with key " + request->key() + " value " + 
    request->value() + " cf " + request->cf() + " region id " + std::to_string(request->context().region_id()), __FILE__, __LINE__, "Server::RawPut");
    if(!this->st_->Write(request->context(), request))
    {
        Logger::GetInstance()->DEBUG_NEW("err: st write error!", __FILE__, __LINE__, "Server::RawPut");
        return Status::CANCELLED;
    }
    return Status::OK;
}

Status Server::RawDelete(ServerContext* context, const kvrpcpb::RawDeleteRequest* request, kvrpcpb::RawDeleteResponse* response) 
{
    return Status::OK;
}

Status Server::RawScan(ServerContext* context, const kvrpcpb::RawScanRequest* request, kvrpcpb::RawScanResponse* response) 
{
    return Status::OK;
}

Status Server::Snapshot(ServerContext* context, const raft_serverpb::SnapshotChunk* request, Done* response) 
{
    return Status::OK;
}

bool Server::RunLogic() {
    Server service;
    grpc::EnableDefaultHealthCheckService(true);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(this->serverAddress_, grpc::InsecureServerCredentials());

    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    Logger::GetInstance()->DEBUG_NEW("server listening on: " + this->serverAddress_, __FILE__, __LINE__, "Server::RunLogic");

    server->Wait();
}

Server::~Server() {

}

} // namespace kvserver
