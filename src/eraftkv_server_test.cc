/**
 * @file eraftkv_server_test.cc
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-04-01
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "eraftkv_server.h"

#include <grpcpp/grpcpp.h>

#include <iostream>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

int main() {
  grpc::ChannelArguments args;
  std::vector<
      std::unique_ptr<grpc::experimental::ClientInterceptorFactoryInterface>>
      interceptor_creators;
  interceptor_creators.push_back(std::unique_ptr<CachingInterceptorFactory>(
      new CachingInterceptorFactory()));
  auto channel = grpc::experimental::CreateCustomChannelWithInterceptors(
      "localhost:50051",
      grpc::InsecureChannelCredentials(),
      args,
      std::move(interceptor_creators));

  std::unique_ptr<ERaftKv::Stub> stub_(ERaftKv::NewStub(channel));
  ClientContext                  context;
  eraftkv::RequestVoteReq        req;
  eraftkv::RequestVoteResp       resp;
  auto status = stub_->RequestVote(&context, req, &resp);
  std::cout << resp.vote_granted << std::endl;
  return 0;
}