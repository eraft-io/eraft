#ifndef ERAFT_KV_RAFT_CLIENT_H_
#define ERAFT_KV_RAFT_CLIENT_H_

#include <grpcpp/grpcpp.h>
#include <eraftio/tinykvpb.grpc.pb.h>
#include <eraftio/raft_serverpb.pb.h>
#include <mutex>
#include <memory>
#include <string>
#include <map>
#include <Kv/Config.h>

namespace kvserver
{

class RaftConn
{
public:

    RaftConn(std::string addr_, Config* cfg);

    ~RaftConn();

    void Stop();

    bool Send(raft_serverpb::RaftMessage* msg);

private:

    std::mutex chanMu_;

    std::shared_ptr<grpc::Channel> chan_;

};

class RaftClient
{

public:
   
    RaftClient(Config *c);

    RaftConn* GetConn(std::string addr, uint64_t regionID);

    bool Send(uint64_t storeID, std::string addr, raft_serverpb::RaftMessage* msg);

    std::string GetAddr(uint64_t storeID);

    void InsertAddr(uint64_t storeID, std::string addr);

    void Flush();

    ~RaftClient();

private:
    /* data */
    Config* conf_;

    std::mutex mu_;

    std::map<std::string, RaftConn*> conns_;

    std::map<uint64_t, std::string> addrs_;

};


} // namespace kvserver


#endif