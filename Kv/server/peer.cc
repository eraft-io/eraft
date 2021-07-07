#include <Kv/peer.h>
#include <Kv/peer_storage.h>

#include <RaftCore/Raft.h>
#include <RaftCore/RawNode.h>

#include <memory>

namespace kvserver
{

Peer::Peer(uint64_t storeID, std::shared_ptr<Config> cfg, std::shared_ptr<Engines> engines, std::shared_ptr<metapb::Region> region, std::shared_ptr<metapb::Peer> meta)
{
    assert(meta->id() == 0);
    std::string tag = "region->id() + meta->id()"; // TODO: sprintf to str
    std::shared_ptr<PeerStorage> ps = std::make_shared<PeerStorage>(engines, region, tag);
    
    uint64_t appliedIndex = ps->AppliedIndex();

    eraft::Config nodeConf(meta->id(), cfg->raftElectionTimeoutTicks_, cfg->raftHeartbeatTicks_, appliedIndex, ps);
    std::shared_ptr<eraft::RawNode> raftGroup = std::make_shared<eraft::RawNode>(nodeConf);

    this->meta_ = meta;
    this->regionId_ = region->id();
    this->raftGroup_ = raftGroup;
    this->peerStorage_ = ps;
    this->tag_ = tag;

    // TODO: ticker
    if(region->peers().size() == 1 && region->peers(0).store_id() == storeID) {
        this->raftGroup_->Campaign();
    }
}

Peer::~Peer()
{

}

void Peer::InsertPeerCache(std::shared_ptr<metapb::Peer> peer)
{
    this->peerCache_[peer->id()] = peer;
}

void Peer::RemovePeerCache(uint64_t peerID)
{
    this->peerCache_.erase(peerID);
}

std::shared_ptr<metapb::Peer> Peer::GetPeerFromCache(uint64_t peerID)
{
    if(this->peerCache_.find(peerID) != this->peerCache_.end()) {
        return this->peerCache_[peerID];
    }
    for(auto peer : peerStorage_->region_->peers()) {
        if(peer.id() == peerID) {
            this->InsertPeerCache(std::move(peer));
            return std::move(peer);
        }
    }
    return nullptr;
}

uint64_t Peer::NextProposalIndex()
{
    return this->raftGroup_->raft->raftLog_->LastIndex() + 1;
}

bool Peer::MaybeDestory()
{

}

bool Peer::Destory(std::shared_ptr<Engines> engine, bool keepData)
{

}

bool Peer::IsInitialized()
{
    // this->peerStorage_->
}

uint64_t Peer::storeID()
{
    return this->meta_->store_id();
}

std::shared_ptr<metapb::Region> Peer::Region()
{

}

void Peer::SetRegion(std::shared_ptr<metapb::Region> region)
{

}

uint64_t Peer::PeerId()
{

}

uint64_t Peer::LeaderId()
{

}

bool Peer::IsLeader()
{

}

void Peer::Send(std::shared_ptr<Transport> trans, std::vector<eraftpb::Message> msgs)
{

}

std::vector<std::shared_ptr<metapb::Peer> > Peer::CollectPendingPeers()
{

}

void Peer::ClearPeersStartPendingTime()
{

}

bool Peer::AnyNewPeerCatchUp(uint64_t peerId)
{

}

bool Peer::MaydeCampaign(bool parentIsLeader)
{

}

uint64_t Peer::Term()
{

}

void Peer::HeartbeatScheduler()
{

}

bool Peer::SendRaftMessage(eraftpb::Message msg, std::shared_ptr<Transport> trans)
{
    
}
    
} // namespace kvserver
