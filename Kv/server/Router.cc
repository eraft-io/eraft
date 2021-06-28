#include <Kv/Router.h>

namespace kvserver
{

Router::Router(std::deque<Msg> storeSender) {
    this->storeSender_ = storeSender;
}
    
PeerState* Router::Get(uint64_t regionID){
    if(this->peers_.find(regionID) != this->peers_.end()) {
        return this->peers_[regionID];
    }
    return nullptr;
}

void Router::Register(Peer* peer) {
    PeerState* ps = new PeerState(peer);
    this->peers_[peer->regionId_] = ps;
}

void Router::Close(uint64_t regionID) {

}

bool Router::Send(uint64_t regionID, Msg msg) {

}

void Router::SendStore(Msg m) {

}

} // namespace kvserver
