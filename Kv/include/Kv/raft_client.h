#ifndef ERAFT_KV_RAFT_CLIENT_H_
#define ERAFT_KV_RAFT_CLIENT_H_

#include <grpcpp/grpcpp.h>
#include <eraftio/tinykvpb.grpc.pb.h>
#include <eraftio/raft_serverpb.pb.h>
#include <mutex>
#include <memory>
#include <string>
#include <map>

#include <Kv/config.h>

namespace kvserver
{

class RaftConn
{
public:

    RaftConn(std::string addr_, std::shared_ptr<Config> cfg);

    ~RaftConn();

    void Stop();

    bool Send(raft_serverpb::RaftMessage& msg);

    std::shared_ptr<grpc::Channel> GetChan();

private:

    std::mutex chanMu_;

    std::shared_ptr<grpc::Channel> chan_;

};


class RaftClient
{

public:
   
    RaftClient(std::shared_ptr<Config> c);
    ~RaftClient();

    std::shared_ptr<RaftConn> GetConn(std::string addr, uint64_t regionID);

    bool Send(uint64_t storeID, std::string addr, raft_serverpb::RaftMessage& msg);

    std::string GetAddr(uint64_t storeID);
    
    void InsertAddr(uint64_t storeID, std::string addr);

    void Flush();

private:
    /* data */
    std::shared_ptr<Config> conf_;

    std::mutex mu_;

    std::map<std::string, std::shared_ptr<RaftConn> > conns_;

    std::map<uint64_t, std::string> addrs_;

};


} // namespace kvserver


#endif