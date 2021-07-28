// @file RawNode.cc
// @author Colin
// This module impl some eraft raft core class.
// 
// Inspired by etcd golang version.

#include <RaftCore/RawNode.h>
#include <RaftCore/Util.h>
#include <Logger/Logger.h>

namespace eraft
{

    RawNode::RawNode(Config& config) {
        this->raft = std::make_shared<RaftContext>(config);
        this->prevSoftSt = this->raft->SoftState();
        this->prevHardSt = this->raft->HardState();
    }

    void RawNode::Tick() {
        this->raft->Tick();
    }

    void RawNode::Campaign() {
        Logger::GetInstance()->DEBUG_NEW("rawnode start campaign", __FILE__, __LINE__, "RawNode::Campaign");
        eraftpb::Message msg;
        msg.set_msg_type(eraftpb::MsgHup);
        this->raft->Step(msg);
    }

    void RawNode::Propose(std::string data) {
        eraftpb::Message msg;
        msg.set_msg_type(eraftpb::MsgPropose);
        msg.set_from(this->raft->id_);
        msg.set_temp_data(data);
        this->raft->Step(msg);
    }

    void RawNode::ProposeConfChange(eraftpb::ConfChange cc) {
        std::string data = cc.SerializeAsString();
        eraftpb::Entry ent;
        ent.set_entry_type(eraftpb::EntryConfChange);
        ent.set_data("conf");
        eraftpb::Message msg;
        msg.set_msg_type(eraftpb::MsgPropose);
        this->raft->Step(msg);
    }

    eraftpb::ConfState RawNode::ApplyConfChange(eraftpb::ConfChange cc) {
        eraftpb::ConfState confState;
        if(cc.node_id() == NONE) {
            std::vector<uint64_t> nodes = this->raft->Nodes(this->raft);
            for(uint64_t i = 0; i < nodes.size(); i++) {
                confState.set_nodes(i, nodes[i]);
            }
        }
        switch (cc.change_type())
        {
        case eraftpb::AddNode:
            {
                this->raft->AddNode(cc.node_id());
                break;
            }
        case eraftpb::RemoveNode:
            {
                this->raft->RemoveNode(cc.node_id());
                break;
            }
        }
        std::vector<uint64_t> nodes = this->raft->Nodes(this->raft);
            for(uint64_t i = 0; i < nodes.size(); i++) {
                confState.set_nodes(i, nodes[i]);
            }
        return confState;
    }

    void RawNode::Step(eraftpb::Message m) {
        this->raft->Step(m);
    }

    DReady RawNode::EReady() {
        std::shared_ptr<RaftContext> r = this->raft;
        DReady rd;
        rd.entries = r->raftLog_->UnstableEntries();
        rd.committedEntries = r->raftLog_->NextEnts();
        rd.messages = r->msgs_;
        // if(!r->SoftState()->Equal(this->prevSoftSt)) {
        //     this->prevSoftSt = r->SoftState();
        //     rd.softSt = r->SoftState();
        // }
        // if(!IsHardStateEqual(*r->HardState(), *this->prevHardSt)) {
        //     rd.hardSt = *r->HardState();
        // }
        this->raft->msgs_.clear();
        // if(!IsEmptySnap(r->raftLog_->pendingSnapshot_)) {
        //     rd.snapshot = r->raftLog_->pendingSnapshot_;
        //     r->raftLog_->pendingSnapshot_.clear_data();
        // }
        return rd;
    }

    bool RawNode::HasReady() {
        if(!IsEmptyHardState(*this->raft->HardState()) && !IsHardStateEqual(*this->raft->HardState(), *this->prevHardSt)) {
            return true;
        }
        if(this->raft->raftLog_->UnstableEntries().size() > 0 || this->raft->raftLog_->NextEnts().size() > 0 || this->raft->msgs_.size() > 0) {
            return true;
        }
        if(!IsEmptySnap(this->raft->raftLog_->pendingSnapshot_)) {
            return true;
        }
        return false;
    }

    void RawNode::Advance(DReady rd) {
        if(!IsEmptyHardState(rd.hardSt)) {
            this->prevHardSt = std::make_shared<eraftpb::HardState>(rd.hardSt);
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
