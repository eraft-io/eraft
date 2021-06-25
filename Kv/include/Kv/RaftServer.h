#ifndef ERAFT_KV_RAFT_SERVER_H_
#define ERAFT_KV_RAFT_SERVER_H_

#include <Kv/Engines.h>
#include <Kv/Config.h>

namespace kvserver
{
    
class RaftStorage
{

public:
    RaftStorage(/* args */);
    ~RaftStorage();

private:
    /* data */
    Engines* engs_;

    Config* conf_;


};


}



} // namespace kvserver



#endif