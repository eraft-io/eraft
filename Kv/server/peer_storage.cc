#include <Kv/peer_storage.h>
#include <RaftCore/Util.h>
#include <Kv/utils.h>

namespace kvserver {

PeerStorage::PeerStorage(std::shared_ptr<Engines> engs, std::shared_ptr<metapb::Region> region, std::string tag)
{
    // TODO: log craating storage for region
    auto raftStatePair = Assistant::GetInstance()->InitRaftLocalState(engs->raftDB_, region);
    assert(raftStatePair.second); // if creating error, abort

    auto applyStatePair = Assistant::GetInstance()->InitApplyState(engs->kvDB_, region);
    assert(applyStatePair.second); // if creating error, abort

    if(raftStatePair.first->last_index() < applyStatePair.first->applied_index())
    {
        // unexpected raft log index
        assert(false);
    }

    this->engines_ = engs;
    this->region_ = region;
    this->tag_ = tag;
    this->raftState_ = raftStatePair.first;
    this->applyState_ = applyStatePair.first;
    // TODO: region sched
}

PeerStorage::~PeerStorage()
{

}

// InitialState implements the Storage interface.
std::pair<eraftpb::HardState, eraftpb::ConfState> PeerStorage::InitialState()
{
    if(eraft::IsEmptyHardState(this->raftState_->hard_state()))
    {
        return std::pair<eraftpb::HardState, eraftpb::ConfState>(eraftpb::HardState(), eraftpb::ConfState());
    }
    return std::pair<eraftpb::HardState, eraftpb::ConfState> (this->raftState_->hard_state(), Assistant::GetInstance()->ConfStateFromRegion(this->region_));
}

// Entries implements the Storage interface.
std::vector<eraftpb::Entry> PeerStorage::Entries(uint64_t lo, uint64_t hi)
{
    // TODO: check range
    std::vector<eraftpb::Entry> ents;

    std::string startKey = std::string(Assistant::GetInstance()->RaftLogKey(this->region_->id(), lo).begin(), Assistant::GetInstance()->RaftLogKey(this->region_->id(), lo).end());
    std::string endKey = std::string(Assistant::GetInstance()->RaftLogKey(this->region_->id(), hi).begin(), Assistant::GetInstance()->RaftLogKey(this->region_->id(), hi).end());

    uint64_t nextIndex = lo;

    auto iter = this->engines_->raftDB_->NewIterator(rocksdb::ReadOptions());
    for(iter->Seek(startKey); iter->Valid(); iter->Next())
    {
        if(Assistant::GetInstance()->ExceedEndKey(iter->key().ToString(), endKey))
        {
            break;
        }
        std::string val = iter->value().ToString();
        eraftpb::Entry ent;
        ent.ParseFromString(val);

        if(ent.index() != nextIndex)
        {
            break;
        }

        nextIndex++;
        ents.push_back(ent);
    }
    return ents;
}

// Term implements the Storage interface.
uint64_t PeerStorage::Term(uint64_t idx)
{
    if(idx == this->TruncatedIndex())
    {
        return this->TruncatedTerm();
    }
    // TODO: check idx, idx+1
    if(this->TruncatedTerm() == this->raftState_->last_term() || idx == this->raftState_->last_index())
    {
        return this->raftState_->last_term();
    }
    eraftpb::Entry* entry;
    Assistant::GetInstance()->GetMeta(this->engines_->raftDB_, std::string(Assistant::GetInstance()->RaftLogKey(this->region_->id(), idx).begin(), 
        Assistant::GetInstance()->RaftLogKey(this->region_->id(), idx).end()), entry);
    return entry->term();
}

// LastIndex implements the Storage interface.
uint64_t PeerStorage::LastIndex()
{
    return this->raftState_->last_index();
}

// FirstIndex implements the Storage interface.
uint64_t PeerStorage::FirstIndex()
{
    return this->TruncatedIndex() + 1;
}

// Snapshot implements the Storage interface.
eraftpb::Snapshot PeerStorage::Snapshot()
{
    // TODO: 
}

// SetHardState saves the current HardState.
void PeerStorage::SetHardState(eraftpb::HardState &st)
{

}

// ApplySnapshot overwrites the contents of this Storage object with
// those of the given snapshot.
bool PeerStorage::ApplySnapshot(eraftpb::Snapshot &snap)
{

}

// CreateSnapshot makes a snapshot which can be retrieved with Snapshot() and
// can be used to reconstruct the state at that point.
// If any configuration changes have been made since the last compaction,
// the result of the last ApplyConfChange must be passed in.
eraftpb::Snapshot PeerStorage::CreateSnapshot(uint64_t i, eraftpb::ConfState* cs, const char* bytes)
{

}

// Compact discards all log entries prior to compactIndex.
// It is the application's responsibility to not attempt to compact an index
// greater than raftLog.applied.
bool PeerStorage::Compact(uint64_t compactIndex)
{

}

// Append the new entries to storage.
// entries[0].Index > ms.entries[0].Index
bool PeerStorage::Append(std::vector<eraftpb::Entry> entries, std::shared_ptr<rocksdb::WriteBatch> raftWB)
{
    if(entries.size() == 0) 
    {
        return false;
    }

    uint64_t first = this->FirstIndex();
    uint64_t last = entries[entries.size()-1].index();

    if(last < first)
    {
        return false;
    }

    if(first > entries[0].index())
    {
        entries.erase(entries.begin() + (first-entries[0].index()));
    }

    uint64_t regionId = this->region_->id();
    for(auto entry : entries) 
    {
        Assistant::GetInstance()->SetMeta(raftWB.get(), std::string(Assistant::GetInstance()->RaftLogKey(regionId, entry.index()).begin(), Assistant::GetInstance()->RaftLogKey(regionId, entry.index()).end() ), entry);   
    }

    uint64_t prevLast = this->LastIndex();
    if(prevLast > last) 
    {
        for(uint64_t i = last + 1; i <= prevLast; i++)
        {
            raftWB->Delete(std::string(Assistant::GetInstance()->RaftLogKey(regionId, i).begin(), Assistant::GetInstance()->RaftLogKey(regionId, i).end()));
        }
    }

    this->raftState_->set_last_index(last);
    this->raftState_->set_last_term(entries[entries.size()-1].term());

    return true;
}

uint64_t PeerStorage::AppliedIndex()
{
    return this->applyState_->applied_index();
}

bool PeerStorage::IsInitialized()
{
    return (this->region_->peers().size() > 0);  
}

std::shared_ptr<metapb::Region> PeerStorage::Region()
{
    return this->region_;
}

void PeerStorage::SetRegion(std::shared_ptr<metapb::Region> region)
{
    this->region_ = region;
}

bool PeerStorage::CheckRange(uint64_t low, uint64_t high)
{
    if(low > high)
    {
        return false;
    } 
    else if (low <= this->TruncatedIndex())
    {
        return false;
    }
    else if (high > this->raftState_->last_index()+1)
    {
        return false;
    }
    return true;
}

uint64_t PeerStorage::TruncatedIndex()
{
    return this->applyState_->truncated_state().index();
}

uint64_t PeerStorage::TruncatedTerm()
{
    return this->applyState_->truncated_state().term();
}

bool PeerStorage::ValidateSnap(std::shared_ptr<eraftpb::Snapshot> snap)
{
    // TODO: check snap
}

bool PeerStorage::ClearMeta(std::shared_ptr<rocksdb::WriteBatch> kvWB, std::shared_ptr<rocksdb::WriteBatch> raftWB)
{
    return Assistant::GetInstance()->DoClearMeta(this->engines_, kvWB, raftWB, this->region_->id(), this->raftState_->last_index());
}

// delete all data that is not covered by new_region
void PeerStorage::ClearExtraData(std::shared_ptr<metapb::Region> newRegion)
{
    if(this->region_->start_key().compare(newRegion->start_key()) < 0) 
    {
        this->ClearRange(newRegion->id(), this->region_->start_key(), newRegion->start_key());
    }
    if(newRegion->end_key().compare(this->region_->end_key()) < 0)
    {
        this->ClearRange(newRegion->id(), newRegion->end_key(), this->region_->end_key());
    }
}

// save memory states to disk
std::shared_ptr<ApplySnapResult> PeerStorage::SaveReadyState(std::shared_ptr<eraft::DReady> ready)
{
    std::shared_ptr<rocksdb::WriteBatch> raftWB = std::make_shared<rocksdb::WriteBatch>();
    ApplySnapResult result;
    if(!eraft::IsEmptySnap(ready->snapshot))
    {
        // TODO: apply snap
    }

    this->Append(ready->entries, raftWB);

    if(eraft::IsEmptyHardState(ready->hardSt))
    {
        this->raftState_->set_allocated_hard_state(&ready->hardSt);
    }
    Assistant::GetInstance()->SetMeta(raftWB.get(), std::string(Assistant::GetInstance()->RaftStateKey(this->region_->id()).begin(), Assistant::GetInstance()->RaftStateKey(this->region_->id()).end()), *this->raftState_);
    this->engines_->raftDB_->Write(rocksdb::WriteOptions(), & *raftWB);
    return std::make_shared<ApplySnapResult>(result);
}

void PeerStorage::ClearData()
{
    this->ClearRange(this->region_->id(), this->region_->start_key(), this->region_->end_key());
}

void PeerStorage::ClearRange(uint64_t regionID, std::string start, std::string end)
{
    // sched region destory task
}


} // namespace kvserver