#include <Kv/ServerImpl.h>
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
