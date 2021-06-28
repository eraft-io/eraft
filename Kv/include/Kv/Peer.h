#ifndef ERAFT_KV_PEER_H_
#define ERAFT_KV_PEER_H_

#include <RaftCore/RawNode.h>
#include <eraftio/metapb.pb.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <Kv/Callback.h>

namespace kvserver
{

class PeerStorage;

struct Proposal
{
    uint64_t index_;

    uint64_t term_;

    Callback* db_;
};


class Peer
{

friend class Router;

public:
    Peer(/* args */);
    ~Peer();

    // ticker

    // instance of the raft moudle
    eraft::RawNode* raftGroup_;

    // peer storage
    PeerStorage* peerStorage_;

    metapb::Peer* meta_;

private:

    uint64_t regionId_;

    std::string tag_;

    std::vector<Proposal> proposals_;

    uint64_t lastCompactedIdx_;

    std::map<uint64_t, metapb::Peer* > peerCache_;
    
    bool stopped_;

    uint64_t sizeDiffHint_;

    uint64_t approximateSize_;

};


} // namespace kvserver


#endif