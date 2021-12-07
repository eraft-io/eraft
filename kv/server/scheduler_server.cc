#include <kv/scheduler_server.h>
#include <spdlog/spdlog.h>

namespace kvserver {

Status ScheServer::GetMembers(ServerContext* context,
                              const schedulerpb::GetMembersRequest* request,
                              schedulerpb::GetMembersResponse* response) {
  auto leader = this->st_->raftSystem_->GetLeader();
  schedulerpb::Member mem;
  response->mutable_etcd_leader()->set_member_id(leader->PeerId());
  response->mutable_etcd_leader()->set_name(leader->meta_->addr());
  return Status::OK;
}

Status ScheServer::Bootstrap(ServerContext* context,
                             const schedulerpb::BootstrapRequest* request,
                             schedulerpb::BootstrapResponse* response) {
  return Status::OK;
}

Status ScheServer::IsBootstrapped(
    ServerContext* context, const schedulerpb::IsBootstrappedRequest* request,
    schedulerpb::IsBootstrappedResponse* response) {
  return Status::OK;
}

Status ScheServer::AllocID(ServerContext* context,
                           const schedulerpb::AllocIDRequest* request,
                           schedulerpb::AllocIDResponse* response) {
  return Status::OK;
}

Status ScheServer::GetStore(ServerContext* context,
                            const schedulerpb::GetStoreRequest* request,
                            schedulerpb::GetStoreResponse* response) {
  return Status::OK;
}

Status ScheServer::PutStore(ServerContext* context,
                            const schedulerpb::PutStoreRequest* request,
                            schedulerpb::PutStoreResponse* response) {
  return Status::OK;
}

Status ScheServer::GetAllStores(ServerContext* context,
                                const schedulerpb::GetAllStoresRequest* request,
                                schedulerpb::GetAllStoresResponse* response) {
  return Status::OK;
}

Status ScheServer::StoreHeartbeat(
    ServerContext* context, const schedulerpb::StoreHeartbeatRequest* request,
    schedulerpb::StoreHeartbeatResponse* response) {
  return Status::OK;
}

Status ScheServer::GetRegion(ServerContext* context,
                             const schedulerpb::GetRegionRequest* request,
                             schedulerpb::GetRegionResponse* response) {
  return Status::OK;
}

Status ScheServer::GetPrevRegion(ServerContext* context,
                                 const schedulerpb::GetRegionRequest* request,
                                 schedulerpb::GetRegionResponse* response) {
  return Status::OK;
}

Status ScheServer::GetRegionByID(
    ServerContext* context, const schedulerpb::GetRegionByIDRequest* request,
    schedulerpb::GetRegionResponse* response) {
  return Status::OK;
}

Status ScheServer::ScanRegions(ServerContext* context,
                               const schedulerpb::ScanRegionsRequest* request,
                               schedulerpb::ScanRegionsResponse* response) {
  return Status::OK;
}

Status ScheServer::AskSplit(ServerContext* context,
                            const schedulerpb::AskSplitRequest* request,
                            schedulerpb::AskSplitResponse* response) {
  return Status::OK;
}

Status ScheServer::GetClusterConfig(
    ServerContext* context, const schedulerpb::GetClusterConfigRequest* request,
    schedulerpb::GetClusterConfigResponse* response) {
  return Status::OK;
}

Status ScheServer::PutClusterConfig(
    ServerContext* context, const schedulerpb::PutClusterConfigRequest* request,
    schedulerpb::PutClusterConfigResponse* response) {
  return Status::OK;
}

Status ScheServer::GetOperator(ServerContext* context,
                               const schedulerpb::GetOperatorRequest* request,
                               schedulerpb::GetOperatorResponse* response) {
  return Status::OK;
}

ScheServer::ScheServer(std::string addr, RaftStorage* st)
    : serverAddress_(addr), st_(st) {}

ScheServer::~ScheServer() {}

ScheServer::ScheServer() {}

bool ScheServer::RunLogic() {
  ScheServer schSvr;
  grpc::EnableDefaultHealthCheckService(true);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(this->serverAddress_,
                           grpc::InsecureServerCredentials());

  builder.RegisterService(&schSvr);

  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

  SPDLOG_INFO("e_meta server listening on: " + this->serverAddress_);

  server->Wait();
}

}  // namespace kvserver