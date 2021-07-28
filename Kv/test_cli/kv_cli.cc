#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <eraftio/helloworld.grpc.pb.h>
#include <eraftio/kvrpcpb.grpc.pb.h>
#include <Kv/raft_client.h>
#include <Kv/config.h>

using namespace kvserver;

int main(int argc, char** argv) {

  std::shared_ptr<Config> conf = std::make_shared<Config>(std::string(argv[1]), 
    std::string(argv[2]), std::stoi(std::string(argv[3])));
  
  std::shared_ptr<RaftClient> raftClient = std::make_shared<RaftClient>(conf);
  kvrpcpb::RawPutRequest request;
  request.mutable_context()->set_region_id(1);
  request.set_cf(std::string(argv[2]));
  request.set_value(std::string(argv[2]));
  request.set_key(std::string(argv[2]));
  raftClient->PutRaw(std::string(argv[1]), request);

  return 0;
}
