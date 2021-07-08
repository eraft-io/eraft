#include <Kv/raft_client.h>

#include <grpcpp/grpcpp.h>
#include <eraftio/tinykvpb.grpc.pb.h>
#include <eraftio/raft_serverpb.pb.h>

using raft_serverpb::Done;
using tinykvpb::TinyKv;

namespace kvserver
{

RaftConn::RaftConn(std::string addr_, std::shared_ptr<Config>) {
    this->chan_ = grpc::CreateChannel(addr_, grpc::InsecureChannelCredentials());
}

RaftConn::~RaftConn() 
{
}

void RaftConn::Stop() {

}

std::shared_ptr<grpc::Channel> RaftConn::GetChan() {
    return this->chan_;
}


RaftClient::RaftClient(std::shared_ptr<Config> c){
    this->conf_ = c;
}

RaftClient::~RaftClient()
{

}

/***
 *  Get connection from cache, if not find, create 
 *  a new one and insert it to cache map
 */

std::shared_ptr<RaftConn> RaftClient::GetConn(std::string addr, uint64_t regionID) {
    if(this->conns_.find(addr) != this->conns_.end()) {
        return this->conns_[addr];
    }
    std::shared_ptr<RaftConn> newConn = std::make_shared<RaftConn>(addr, this->conf_);
    {
        std::lock_guard<std::mutex> lck (this->mu_);
        this->conns_[addr] = newConn;
    }
    return newConn;
}

bool RaftClient::Send(uint64_t storeID, std::string addr, raft_serverpb::RaftMessage& msg) 
{
    std::shared_ptr<RaftConn> conn = this->GetConn(addr, msg.region_id());
    std::unique_ptr<TinyKv::Stub> stub_(TinyKv::NewStub(conn->GetChan()));
    Done done;
    grpc::ClientContext context;
    stub_->Raft(&context, msg, &done);
    {
        std::lock_guard<std::mutex> lck (this->mu_);
        conns_.erase(addr);
    }
    
    return true;
}

std::string RaftClient::GetAddr(uint64_t storeID) {
    return this->addrs_[storeID];
}

void RaftClient::InsertAddr(uint64_t storeID, std::string addr) {
    {
        std::lock_guard<std::mutex> lck (this->mu_);
        this->addrs_[storeID] = addr;
    }
}

void RaftClient::Flush() {

}
   

} // namespace kvserver
