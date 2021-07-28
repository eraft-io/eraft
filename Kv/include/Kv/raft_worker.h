#ifndef ERAFT_KV_RAFT_WORKER_H_
#define ERAFT_KV_RAFT_WORKER_H_

#include <deque>
#include <memory>
#include <map>

#include <Kv/raft_store.h>
#include <Kv/msg.h>
#include <Kv/router.h>
#include <Kv/concurrency_queue.h>

namespace kvserver
{

struct GlobalContext;
struct Router;
struct PeerState_;

class RaftWorker
{

public:

    RaftWorker(std::shared_ptr<GlobalContext> ctx, std::shared_ptr<Router> pm);
    ~RaftWorker();

    static void Run(Queue<Msg>& qu);

    static void BootThread();

    static std::shared_ptr<PeerState_> GetPeerState(std::map<uint64_t, std::shared_ptr<PeerState_> > peersStateMap, uint64_t regionID);

private:

    static std::shared_ptr<Router> pr_;
    
    static std::shared_ptr<GlobalContext> ctx_;

};


} // namespace kvserver


#endif