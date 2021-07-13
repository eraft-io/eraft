#include <Kv/peer.h>
#include <Kv/peer_storage.h>

#include <RaftCore/Raft.h>
#include <RaftCore/RawNode.h>
#include <rocksdb/write_batch.h>

#include <eraftio/raft_serverpb.pb.h>
#include <Kv/utils.h>

#include <memory>

namespace kvserver
{

Peer::Peer(uint64_t storeID, std::shared_ptr<Config> cfg, std::shared_ptr<Engines> engines, std::shared_ptr<metapb::Region> region)
{
    std::shared_ptr<metapb::Peer> meta;
    // find peer
    assert(meta->id() == 0);
    std::string tag = "[region " + std::to_string(region->id()) + " ] " + std::to_string(meta->id());
    // TODO: sprintf to str
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

void Peer::InsertPeerCache(metapb::Peer* peer)
{
    this->peerCache_.insert(std::pair<uint64_t, std::shared_ptr<metapb::Peer> >(peer->id(), peer));
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
            this->InsertPeerCache(&peer);
            return std::make_shared<metapb::Peer>(peer);
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
    if(this->stopped_) {
        return false;
    }
    return true;
}

bool Peer::Destory(std::shared_ptr<Engines> engine, bool keepData)
{
    // TODO: cal the destory cost time

    std::shared_ptr<metapb::Region> region = this->Region();
    // TODO: // log p.tag begin to destory

    std::shared_ptr<rocksdb::WriteBatch> kvWB = std::make_shared<rocksdb::WriteBatch>();
    std::shared_ptr<rocksdb::WriteBatch> raftWB = std::make_shared<rocksdb::WriteBatch>();

    if(!this->peerStorage_->ClearMeta(kvWB, raftWB))
    {
        return false;
    }

    // write region state
    WriteRegionState(kvWB, region, raft_serverpb::PeerState::Tombstone);

    // write state to real db
    engine->kvDB_->Write(rocksdb::WriteOptions(), & *kvWB);
    engine->raftDB_->Write(rocksdb::WriteOptions(), & *raftWB);

    if(this->peerStorage_->IsInitialized() && !keepData) {
        this->peerStorage_->ClearData();
    }

    for(auto proposal: this->proposals_)
    {
        // TODO: notify req region removed
    }
    this->proposals_.clear();

    return true;
}

bool Peer::IsInitialized()
{
    this->peerStorage_->IsInitialized();
}

uint64_t Peer::storeID()
{
    return this->meta_->store_id();
}

std::shared_ptr<metapb::Region> Peer::Region()
{
    return this->peerStorage_->Region();   
}

void Peer::SetRegion(std::shared_ptr<metapb::Region> region)
{
    this->peerStorage_->SetRegion(region);
}

uint64_t Peer::PeerId()
{
    return this->meta_->id();
}

uint64_t Peer::LeaderId()
{
    return this->raftGroup_->raft->lead_;
}

bool Peer::IsLeader()
{
    return this->raftGroup_->raft->state_ == eraft::NodeState::StateLeader;
}

void Peer::Send(std::shared_ptr<Transport> trans, std::vector<eraftpb::Message> msgs)
{
    for(auto msg : msgs) 
    {
        if(!this->SendRaftMessage(msg, trans)) 
        {
            // TODO: log send message err
        }
    }
}

std::vector<std::shared_ptr<metapb::Peer> > Peer::CollectPendingPeers()
{
    std::vector<std::shared_ptr<metapb::Peer> > pendingPeers;
    uint64_t truncatedIdx = this->peerStorage_->TruncatedIndex();
    for(auto pr: this->raftGroup_->GetProgress()) 
    {
        if(pr.first == this->meta_->id())
        {
            continue;
        }
        if(pr.second.match < truncatedIdx) {
            std::shared_ptr<metapb::Peer> peer = this->GetPeerFromCache(pr.first);
            if(peer != nullptr)
            {
                pendingPeers.push_back(peer);
                // TODO: stat peer start pending time
            }
        }
    }
    return pendingPeers;
}

void Peer::ClearPeersStartPendingTime()
{
    //TODO: clear peers start pending time map
}

bool Peer::AnyNewPeerCatchUp(uint64_t peerId)
{

}

bool Peer::MaydeCampaign(bool parentIsLeader)
{
    if(this->Region()->peers().size() <= 1 || !parentIsLeader) {
        return false;
    }

    this->raftGroup_->Campaign();
    return true;
}

uint64_t Peer::Term()
{
    return this->raftGroup_->raft->term_;
}

void Peer::HeartbeatScheduler()
{
    // TODO: tick scheduler heartbeat
}

bool Peer::SendRaftMessage(eraftpb::Message msg, std::shared_ptr<Transport> trans)
{
    std::shared_ptr<raft_serverpb::RaftMessage> sendMsg = std::make_shared<raft_serverpb::RaftMessage>();
    sendMsg->set_region_id(this->regionId_);
    metapb::RegionEpoch epoch;
    epoch.set_conf_ver(this->Region()->region_epoch().conf_ver());
    epoch.set_version(this->Region()->region_epoch().version());
    sendMsg->set_allocated_region_epoch(&epoch);

    auto fromPeer = this->meta_;
    auto toPeer = this->GetPeerFromCache(msg.to());
    if(toPeer == nullptr)
    {
        // TODO: failed to lookup recipient peer
        return false;
    }
    // TODO: log send raft msg from %v to %v

    sendMsg->set_allocated_from_peer(& *fromPeer);
    sendMsg->set_allocated_to_peer(& *toPeer);

    if(this->peerStorage_->IsInitialized() && IsInitialMsg(msg))
    {
        sendMsg->set_start_key(this->Region()->start_key());
        sendMsg->set_end_key(this->Region()->end_key());
    }

    sendMsg->set_allocated_message(&msg);

    return trans->Send(sendMsg);
}
    
} // namespace kvserver
