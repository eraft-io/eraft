#ifndef ERAFT_KV_RAFT_WORKER_H_
#define ERAFT_KV_RAFT_WORKER_H_

#include <Kv/RaftStore.h>
#include <Kv/Router.h>

namespace kvserver
{

class RaftWorker
{
public:
    RaftWorker(GlobalContext *ctx, Router* pm);
    ~RaftWorker();

    void Run();

private:
    // router
    Router *pr_;
    
    // raft message Ch
    

    // close ch 

};


} // namespace kvserver


#endif