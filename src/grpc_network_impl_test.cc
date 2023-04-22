#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <iostream>

#include "eraftkv.grpc.pb.h"
#include "eraftkv.pb.h"
#include "raft_server.h"
#include "grpc_network_impl.h"
#include "eraftkv_server.h"
#include "raft_node.h"
using eraftkv::ERaftKv;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

TEST(GrpcTest, TestInit) {
  pid_t fpid;
  fpid = fork();
  if (fpid < 0) {
    std::cout << "fork error!\n";
  }
  if (fpid == 0) {
    sleep(1);
    std::map<int, std::string> peers_address;
    peers_address[1] = "0.0.0.0:50051";
    GRpcNetworkImpl grpcimpl;
    grpcimpl.InitPeerNodeConnections(peers_address);
    RaftNode raftNode(1, NodeStateEnum::Running, 0, 0);
    RaftConfig raftconf;
    RaftServer* raftServer = new RaftServer(raftconf);
    eraftkv::RequestVoteReq        req;
    auto status = grpcimpl.SendRequestVote(raftServer, &raftNode, &req);
    int  res = 0;
    if (EStatus::kOk == status)
      res = 1;
    ASSERT_EQ(res, 1);
  } else {
    ERaftKvServer service;
    grpc::EnableDefaultHealthCheckService(true);
    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051",
                             grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    sleep(5);
  }
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}