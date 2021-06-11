#include <iostream>
#include <string>
#include <grpcpp/grpcpp.h>
#include <eraftio/tinykvpb.grpc.pb.h>
#include <eraftio/raft_serverpb.pb.h>

using raft_serverpb::Done;
using tinykvpb::TinyKv;

namespace kvserver
{

class KvClient
{

public:

    KvClient(std::shared_ptr<grpc::Channel> channel) : stub_(TinyKv::NewStub(channel)) { }

    bool SendMsg() {

        raft_serverpb::RaftMessage msg_;

        msg_.set_start_key("start_key !!");

        Done done;

        grpc::ClientContext context;

        grpc::Status status = stub_->Raft(&context, msg_, &done);

        return false;
    }


    ~KvClient() {
        
    }

private:
    /* data */
    std::unique_ptr<TinyKv::Stub> stub_;
};
    
} // namespace kvserver


int main(int argc, char** argv) {
    std::string target_str = "127.0.0.1:12306";

    kvserver::KvClient kvCli(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    kvCli.SendMsg();

    return 0;
}
