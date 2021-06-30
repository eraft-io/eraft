#include <Kv/peer.h>

namespace kvserver
{

Peer::Peer(uint64_t storeID, std::shared_ptr<Config> cfg, std::shared_ptr<Engines> engines, std::shared_ptr<metapb::Region> region)
{

}

Peer::~Peer()
{

}

void Peer::InsertPeerCache(std::shared_ptr<metapb::Peer> peer)
{

}

void Peer::RemovePeerCache(uint64_t peerID)
{

}

std::shared_ptr<metapb::Peer> Peer::GetPeerFromCache(uint64_t peerID)
{

}

uint64_t Peer::NextProposalIndex()
{

}

bool Peer::MaybeDestory()
{

}

bool Peer::Destory(std::shared_ptr<Engines> engine, bool keepData)
{

}

bool Peer::IsInitialized()
{

}

uint64_t Peer::storeID()
{

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
