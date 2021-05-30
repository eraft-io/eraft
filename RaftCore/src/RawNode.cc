#include <RaftCore/RawNode.h>
#include <RaftCore/Util.h>

namespace eraft
{

    RawNode::RawNode(Config* config) {
        RaftContext r = RaftContext(config);
        this->raft = &r;
        this->prevSoftSt = r.SoftState();
        this->prevHardSt = r.HardState();
    }

    void RawNode::Tick() {
        this->raft->Tick();
    }

    void RawNode::Campaign() {
        eraftpb::Message msg;
        msg.set_msg_type(eraftpb::MsgHup);
        this->raft->Step(msg);
    }

    void RawNode::Propose(std::vector<uint8_t> *data) {
        eraftpb::Entry ent;
        ent.set_data((const char*)data);
        eraftpb::Message msg;
        msg.set_msg_type(eraftpb::MsgPropose);
        msg.set_from(this->raft->id_);
        msg.add_entries(); // TODO:
        this->raft->Step(msg);
    }

    void RawNode::ProposeConfChange(eraftpb::ConfChange cc) {
        std::string data = cc.SerializeAsString();
        eraftpb::Entry ent;
        ent.set_entry_type(eraftpb::EntryConfChange);
        ent.set_data(data);
        eraftpb::Message msg;
        msg.set_msg_type(eraftpb::MsgPropose);
        this->raft->Step(msg);
    }

    eraftpb::ConfState* RawNode::ApplyConfChange(eraftpb::ConfChange cc) {
        eraftpb::ConfState *confState;
        if(cc.node_id() == NONE) {
            std::vector<uint64_t> nodes = this->raft->Nodes(this->raft);
            for(uint64_t i = 0; i < nodes.size(); i++) {
                confState->set_nodes(i, nodes[i]);
            }
        }
        switch (cc.change_type())
        {
        case eraftpb::AddNode:
            this->raft->AddNode(cc.node_id());
        case eraftpb::RemoveNode:
            this->raft->RemoveNode(cc.node_id());
        default:
            break;
        }
        std::vector<uint64_t> nodes = this->raft->Nodes(this->raft);
            for(uint64_t i = 0; i < nodes.size(); i++) {
                confState->set_nodes(i, nodes[i]);
            }
        return confState;
    }

    void RawNode::Step(eraftpb::Message m) {
        this->raft->Step(m);
    }

    Ready RawNode::EReady() {
        RaftContext* r = this->raft;
        Ready rd;
        rd.entries = r->raftLog_->UnstableEntries();
        rd.committedEntries = r->raftLog_->NextEnts();
        rd.messages = r->msgs_;
        // TODO: check
        ESoftState *softSt = r->SoftState();
        eraftpb::HardState *hardSt = r->HardState();
        if(!softSt->Equal(this->prevSoftSt)) {
            this->prevSoftSt = softSt;
            rd.softSt = softSt;
        }
        if(!IsHardStateEqual(*hardSt, *this->prevHardSt)) {
            rd.hardSt = *hardSt;
        }
        this->raft->msgs_.clear();
        if(!IsEmptySnap(r->raftLog_->pendingSnapshot_)) {
            rd.snapshot = *r->raftLog_->pendingSnapshot_;
            r->raftLog_->pendingSnapshot_ = nullptr;
        }
        return rd;
    }

    bool RawNode::HasReady() {
        RaftContext* r = this->raft; 
        eraftpb::HardState* hardSt = r->HardState();
        if(!IsEmptyHardState(*hardSt) && !IsHardStateEqual(*hardSt, *this->prevHardSt)) {
            return true;
        }
        if(r->raftLog_->UnstableEntries().size() > 0 || r->raftLog_->NextEnts().size() > 0 || r->msgs_.size() > 0) {
            return true;
        }
        if(!IsEmptySnap(r->raftLog_->pendingSnapshot_)) {
            return true;
        }
        return false;
    }

    void RawNode::Advance(Ready rd) {
        if(!IsEmptyHardState(rd.hardSt)) {
            this->prevHardSt = &rd.hardSt;
        }
        if(rd.entries.size() > 0) {
            this->raft->raftLog_->stabled_ = rd.entries[rd.entries.size()-1].index();
        }
        if(rd.committedEntries.size() > 0) {
            this->raft->raftLog_->applied_ = rd.committedEntries[rd.committedEntries.size()-1].index();
        }
        this->raft->raftLog_->MaybeCompact();
    }

    std::map<uint64_t, Progress> RawNode::GetProgress() {
        std::map<uint64_t, Progress> m;
        if(this->raft->state_ == NodeState::StateLeader) {
            for(auto p: this->raft->prs_) {
                m[p.first] = *p.second; 
            }
        }
        return m;
    }

    void RawNode::TransferLeader(uint64_t transferee) {
        eraftpb::Message msg;
        msg.set_msg_type(eraftpb::MsgTransferLeader);
        msg.set_from(transferee);
        this->Step(msg);
    }


} // namespace eraft
