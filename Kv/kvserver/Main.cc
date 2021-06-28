// @file Main.cc
// @author Colin
// This module impl kv main class.
// 

#include <Kv/ServerImpl.h>
#include <Kv/Config.h>

int main(int argc, char *argv[]) {

    kvserver::Config conf;
    conf.PrintConfigToConsole();

    kvserver::Server svr(conf.storeAddr_);
    svr.RunLogic();
    
    return 0;
}
