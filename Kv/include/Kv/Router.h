#ifndef ERAFT_KV_ROUTER_H_
#define ERAFT_KV_ROUTER_H_

#include <Kv/Peer.h>
#include <Kv/Msg.h>
#include <stdint.h>
#include <map>
#include <vector>

namespace kvserver
{

class Peer;

struct Msg;

struct PeerState
{
    uint32_t closed_;

    Peer *peer_;
};


class Router
{
public:
    Router(/* args */);

    Router(std::vector<Msg> storeSender);

    PeerState* Get(uint64_t regionID);

    void Register(Peer* peer);

    void Close(uint64_t regionID);

    bool Send(uint64_t regionID, Msg msg);

    

    ~Router();

protected:

private:

    std::map<uint64_t, Peer*> peers_;

    std::vector<Msg> peerSender_;

    std::vector<Msg> storeSender_;

};



} // namespace kvserver


#endif