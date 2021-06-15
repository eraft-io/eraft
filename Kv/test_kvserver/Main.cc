// @file Main.cc
// @author Colin
// This module impl kv main class.
// 
#include <Kv/ServerImpl.h>

int main(int argc, char *argv[]) {

    kvserver::Server svr;
    svr.RunLogic();
    
    return 0;
}
