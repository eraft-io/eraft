#include <eraftio/tinykvpb.grpc.pb.h>
#include <grpcpp/grpcpp.h>

using tinykvpb::TinyKv;
using grpc::Status;
using grpc::ServerContext;
using grpc::ServerReader;
using raft_serverpb::RaftMessage;
using raft_serverpb::Done;


class ServerRpcImpl : public TinyKv::Service {
    Status Raft(ServerContext* context,  ServerReader<RaftMessage>* reader, Done* response) override {
        RaftMessage* raftMsg;
        reader->Read(raftMsg);
        raftMsg->from_peer();
        raftMsg->to_peer();
        return Status::OK;
    }
};

