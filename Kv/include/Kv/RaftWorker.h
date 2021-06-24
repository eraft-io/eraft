#ifndef ERAFT_KV_RAFT_WORKER_H_
#define ERAFT_KV_RAFT_WORKER_H_

namespace kvserver
{

class RaftWorker
{
public:
    RaftWorker();
    ~RaftWorker();

    void Run();

private:
    // router
    
    // raft message Ch
    
    // close ch 

};


} // namespace kvserver


#endif