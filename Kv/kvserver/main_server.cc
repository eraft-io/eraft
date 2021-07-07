// @file Main.cc
// @author Colin
// This module impl kv main class.
// 

#include <Kv/server_impl.h>
#include <Kv/config.h>
#include <Kv/raft_store.h>
#include <Kv/raft_server.h>

int main(int argc, char *argv[]) {

    // make conf
    std::shared_ptr<kvserver::Config> conf = std::make_shared<kvserver::Config>();
    conf->PrintConfigToConsole();
    
    // start raft store
    kvserver::RaftStorage storage = kvserver::RaftStorage(conf);
    storage.Start();

    // start rpc service server
    kvserver::Server svr(conf->storeAddr_);
    svr.RunLogic();
    
    return 0;
}
