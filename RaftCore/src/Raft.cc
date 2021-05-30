#include <RaftCore/Raft.h>
#include <RaftCore/Util.h>
#include <assert.h>

namespace eraft
{
    bool Config::Validate() {
        if(this->id == 0) {
            // TODO: log cannot use none as id
            return false;
        }
        if(this->heartbeatTick <= 0) {
            // TODO: log heartbeat tick must be greater than 0
            return false;
        }
        if(this->electionTick <= this->heartbeatTick) {
            // TODO: election tick must be greater than heartbeat tick
            return false;
        }
        if(this->storage == nullptr) {
            // TODO: log storage cannot be nil
            return false;
        }
        return true;
    }

    RaftContext::RaftContext(Config *c) {
        assert(c->Validate()); // if Validate config is false, terminating the program execution.
        this->id_ = c->id;
        this->prs_ = std::map<uint64_t, Progress *> {};
        this->votes_ = std::map<uint64_t, bool> {};
        this->heartbeatTimeout_ = c->heartbeatTick;
        this->electionTimeout_ = c->electionTick;
        this->raftLog_ = new RaftLog(*c->storage);
        std::tuple<eraftpb::HardState, eraftpb::ConfState> st(this->raftLog_->storage_->InitialState());
        eraftpb::HardState hardSt = std::get<0>(st);
        eraftpb::ConfState confSt = std::get<1>(st);
        if(c->peers == nullptr) {
            std::vector<uint64_t> *peersTp;
            for(auto node: confSt.nodes()) {
                peersTp->push_back(node);
            }
            c->peers = peersTp;
        }
        uint64_t lastIndex = this->raftLog_->LastIndex();
        for(auto iter = c->peers->begin(); iter != c->peers->end(); iter++) {
            if(*iter == this->id_) {
                this->prs_[*iter] = new Progress{lastIndex + 1, lastIndex};
            } else {
                this->prs_[*iter] = new Progress{lastIndex + 1};
            }
        }
        this->BecomeFollower(0, NONE);
        this->randomElectionTimeout_ = this->electionTimeout_ + RandIntn(this->electionTimeout_);
        this->term_ = hardSt.term();
        this->vote_ = hardSt.vote();
        this->raftLog_->commited_ = hardSt.commit();
        if(c->applied > 0) {
            this->raftLog_->applied_ = c->applied;
        }
    }

    void RaftContext::SendSnapshot(uint64_t to) {
        eraftpb::Snapshot snapshot = this->raftLog_->storage_->Snapshot();
        eraftpb::Message msg;
        msg.set_msg_type(eraftpb::MsgSnapshot);
        msg.set_from(this->id_);
        msg.set_to(to);
        msg.set_term(this->term_);
        msg.set_allocated_snapshot(&snapshot);
        this->msgs_.push_back(msg);
        this->prs_[to]->next = snapshot.metadata().index();
    }

    bool RaftContext::SendAppend(uint64_t to) {
        uint64_t prevIndex = this->prs_[to]->next - 1;
        uint64_t prevLogTerm = this->raftLog_->Term(prevIndex);
        // TODO: if Term() has error
        std::vector<eraftpb::Entry*> entries;
        uint64_t n = this->raftLog_->entries_.size();
        for(uint64_t i = this->raftLog_->ToSliceIndex(prevIndex + 1); i < n; i++) {
            entries.push_back(&this->raftLog_->entries_[i]);
        }
        eraftpb::Message msg;
        msg.set_msg_type(eraftpb::MsgAppend);
        msg.set_from(this->id_);
        msg.set_to(to);
        msg.set_term(this->term_);
        msg.set_commit(this->raftLog_->commited_);
        msg.set_log_term(prevLogTerm);
        msg.set_index(prevIndex);
        for(auto ent: entries) {
            ent = msg.add_entries();
        }
        this->msgs_.push_back(msg);
        return false;
    }

    void RaftContext::SendAppendResponse(uint64_t to, bool reject, uint64_t term, uint64_t index) {
        eraftpb::Message msg;
        msg.set_msg_type(eraftpb::MsgAppendResponse);
        msg.set_from(this->id_);
        msg.set_to(to);
        msg.set_term(this->term_);
        msg.set_reject(reject);
        msg.set_log_term(term);
        msg.set_index(index);
        this->msgs_.push_back(msg);
    }

    void RaftContext::SendHeartbeat(uint64_t to) {
        eraftpb::Message msg;
        msg.set_msg_type(eraftpb::MsgHeartbeat);
        msg.set_from(this->id_);
        msg.set_to(to);
        msg.set_term(this->term_);
        this->msgs_.push_back(msg);
    }

    void RaftContext::SendHeartbeatResponse(uint64_t to, bool reject) {
        eraftpb::Message msg;
        msg.set_msg_type(eraftpb::MsgHeartbeatResponse);
        msg.set_from(this->id_);
        msg.set_to(to);
        msg.set_term(this->term_);
        msg.set_reject(reject);
        this->msgs_.push_back(msg);
    }

    void RaftContext::SendRequestVote(uint64_t to, uint64_t index, uint64_t term) {
        eraftpb::Message msg;
        msg.set_msg_type(eraftpb::MsgRequestVote);
        msg.set_from(this->id_);
        msg.set_to(to);
        msg.set_term(this->term_);
        msg.set_log_term(term);
        msg.set_index(index);
        this->msgs_.push_back(msg);
    }

    void RaftContext::SendRequestVoteResponse(uint64_t to, bool reject) {
        eraftpb::Message msg;
        msg.set_msg_type(eraftpb::MsgRequestVoteResponse);
        msg.set_from(this->id_);
        msg.set_to(to);
        msg.set_term(this->term_);
        msg.set_reject(reject);
        this->msgs_.push_back(msg);
    }

    void RaftContext::SendTimeoutNow(uint64_t to) {
        eraftpb::Message msg;
        msg.set_msg_type(eraftpb::MsgTimeoutNow);
        msg.set_from(this->id_);
        msg.set_to(to);
        this->msgs_.push_back(msg);
    }

    void RaftContext::Tick() {
        switch (this->state_)
        {
        case NodeState::StateFollower:
            this->TickElection();
        case NodeState::StateCandidate:
            this->TickElection();
        case NodeState::StateLeader:
            {
                if(this->leadTransferee_ != NONE) {
                    this->TickTransfer();
                }
                this->TickHeartbeat();
            }
        default:
            break;
        }
    }

    void RaftContext::TickElection() {
        this->electionElapsed_++;
        if(this->electionElapsed_ >= this->randomElectionTimeout_) {
            this->electionElapsed_ = 0;
            eraftpb::Message msg;
            msg.set_msg_type(eraftpb::MsgHup);
            this->Step(msg);
        }
    }

    void RaftContext::TickHeartbeat() {
        this->heartbeatElapsed_++;
        if(this->heartbeatElapsed_ >= this->heartbeatTimeout_) {
            this->heartbeatElapsed_ = 0;
            eraftpb::Message msg;
            msg.set_msg_type(eraftpb::MsgBeat);
            this->Step(msg);
        }
    }

    void RaftContext::TickTransfer() {
        this->transferElapsed_++;
        if(this->transferElapsed_ >= this->electionTimeout_*2) {
            this->transferElapsed_ = 0;
            this->leadTransferee_ = NONE;
        }
    }

    void RaftContext::BecomeFollower(uint64_t term, uint64_t lead) {
        this->state_ = NodeState::StateFollower;
        this->lead_ = lead;
        this->term_ = term;
        this->vote_ = NONE;
    }

    void RaftContext::BecomeCandidate() {
        this->state_ = NodeState::StateCandidate;
        this->lead_ = NONE;
        this->term_ ++;
        this->vote_ = this->id_;
        this->votes_ = std::map<uint64_t, bool>{}; // init
        this->votes_[this->id_] = true; // vote for self
    }

    void RaftContext::BecomeLeader() {
        this->state_ = NodeState::StateLeader;
        this->lead_ = this->id_;
        uint64_t lastIndex = this->raftLog_->LastIndex();
        this->heartbeatElapsed_ = 0;
        for(auto peer : this->prs_) {
            if(peer.first == this->id_) {
                this->prs_[peer.first]->next = lastIndex + 2;
                this->prs_[peer.first]->match = lastIndex + 1; 
            } else {
                this->prs_[peer.first]->next = lastIndex + 1;
            }
        }
        eraftpb::Entry ent;
        ent.set_term(this->term_);
        ent.set_index(this->raftLog_->LastIndex() + 1);
        this->raftLog_->entries_.push_back(ent);
        this->BcastAppend();
        if(this->prs_.size() == 1) {
            this->raftLog_->commited_ = this->prs_[this->id_]->match;
        }
    }

    bool RaftContext::Step(eraftpb::Message m) {
        if(this->prs_.find(this->id_) == this->prs_.end() && m.msg_type() == eraftpb::MsgTimeoutNow) {
            return false;
        }
        if(m.term() > this->term_) {
            this->leadTransferee_ = NONE;
            this->BecomeFollower(m.term(), NONE);
        }
        switch (this->state_)
        {
        case NodeState::StateFollower:
            this->StepFollower(m);
        case NodeState::StateCandidate:
            this->StepCandidate(m);
        case NodeState::StateLeader:
            this->StepLeader(m);
        default:
            break;
        }
        return true;
    }

    // when follower received message, what to do?
    void RaftContext::StepFollower(eraftpb::Message m) {
        switch (m.msg_type())
        {
        case eraftpb::MsgHup:
            this->DoElection();
        case eraftpb::MsgBeat:
        case eraftpb::MsgPropose:
        case eraftpb::MsgAppend:
            this->HandleAppendEntries(m);
        case eraftpb::MsgAppendResponse:
        case eraftpb::MsgRequestVote:
            this->HandleRequestVote(m);
        case eraftpb::MsgRequestVoteResponse:
        case eraftpb::MsgSnapshot:
            this->HandleSnapshot(m);
        case eraftpb::MsgHeartbeat:
            this->HandleHeartbeat(m);
        case eraftpb::MsgHeartbeatResponse:
        case eraftpb::MsgTransferLeader:
            {
                if(this->lead_ != NONE) {
                    m.set_to(this->lead_);
                    this->msgs_.push_back(m);
                }
            }
        case eraftpb::MsgTimeoutNow:
            this->DoElection();
        default:
            break;    
        }
    }

    void RaftContext::StepCandidate(eraftpb::Message m) {
        switch (m.msg_type())
        {
        case eraftpb::MsgHup:
            this->DoElection();
        case eraftpb::MsgBeat:
        case eraftpb::MsgPropose:
        case eraftpb::MsgAppend:
            {
                if(m.term() == this->term_) {
                    this->BecomeFollower(m.term(), m.from());
                }
                this->HandleAppendEntries(m);
            }
        case eraftpb::MsgAppendResponse:
        case eraftpb::MsgRequestVote:
            this->HandleRequestVote(m);
        case eraftpb::MsgRequestVoteResponse:
            this->HandleRequestVoteResponse(m);
        case eraftpb::MsgSnapshot:
            this->HandleSnapshot(m);
        case eraftpb::MsgHeartbeat:
            {
                if(m.term() == this->term_) {
                    this->BecomeFollower(m.term(), m.from());
                }
                this->HandleHeartbeat(m);
            }
        case eraftpb::MsgHeartbeatResponse:
        case eraftpb::MsgTransferLeader:
            {
                if(this->lead_ != NONE) {
                    m.set_to(this->lead_);
                    this->msgs_.push_back(m);
                }
            }
        case eraftpb::MsgTimeoutNow:
        default:
            break;
        }
    }

    void RaftContext::StepLeader(eraftpb::Message m) {
        switch (m.msg_type())
        {
        case eraftpb::MsgHup:
        case eraftpb::MsgBeat:
            this->BcastHeartbeat();
        case eraftpb::MsgPropose:
            {
                if(this->leadTransferee_ == NONE) {
                    std::vector<eraftpb::Entry*> ents;
                    for(auto ent : m.entries()) {
                        ents.push_back(&ent);
                    }
                    this->AppendEntries(ents);
                }
            }
        case eraftpb::MsgAppend:
            this->HandleAppendEntries(m);
        case eraftpb::MsgAppendResponse:
            this->HandleAppendEntriesResponse(m);
        case eraftpb::MsgRequestVote:
            this->HandleRequestVote(m);
        case eraftpb::MsgRequestVoteResponse:
        case eraftpb::MsgSnapshot:
            this->HandleSnapshot(m);
        case eraftpb::MsgHeartbeat:
            this->HandleHeartbeat(m);
        case eraftpb::MsgHeartbeatResponse:
            this->SendAppend(m.from());
        case eraftpb::MsgTransferLeader:
            this->HandleTransferLeader(m);
        case eraftpb::MsgTimeoutNow:
        default:
            break;
        }
    }

    bool RaftContext::DoElection() {
        this->BecomeCandidate();
        this->heartbeatElapsed_ = 0;
        this->randomElectionTimeout_ = this->electionTimeout_ + RandIntn(this->electionTimeout_);
        if(this->prs_.size() == 1) {
            this->BecomeLeader();
            return true;
        }
        uint64_t lastIndex = this->raftLog_->LastIndex();
        uint64_t lastLogTerm = this->raftLog_->Term(lastIndex);
        for(auto peer : this->prs_) {
            if(peer.first == this->id_) {
                continue;
            }
            this->SendRequestVote(peer.first, lastIndex, lastLogTerm);
        }
    }

    void RaftContext::BcastHeartbeat() {
        for(auto peer: this->prs_) {
            if(peer.first == this->id_) {
                continue;
            }
            this->SendHeartbeat(peer.first);
        }
    }

    void RaftContext::BcastAppend() {
        for(auto peer: this->prs_) {
            if(peer.first == this->id_) {
                continue;
            }
            this->SendAppend(peer.first);
        }
    }

    bool RaftContext::HandleRequestVote(eraftpb::Message m) {
        if(m.term() != NONE && m.term() < this->term_) {
            this->SendRequestVoteResponse(m.from(), true);
            return true;
        }
        if(this->vote_ != NONE && this->vote_ != m.from()) {
            this->SendRequestVoteResponse(m.from(), true);
            return true;
        }
        uint64_t lastIndex = this->raftLog_->LastIndex();
        uint64_t lastLogTerm = this->raftLog_->Term(lastIndex);
        if(lastLogTerm > m.log_term() || 
            (lastLogTerm == m.log_term() && lastIndex > m.index())) {
            this->SendRequestVoteResponse(m.from(), true);
            return true;
        }
        this->vote_ = m.from();
        this->electionElapsed_ = 0;
        this->randomElectionTimeout_ = this->electionTimeout_ + RandIntn(this->electionTimeout_);
        this->SendRequestVoteResponse(m.from(), false);
    }

    bool RaftContext::HandleRequestVoteResponse(eraftpb::Message m) {
        if(m.term() != NONE && m.term() < this->term_) {
            return true;
        }
        this->votes_[m.from()] = !m.reject();
        uint8_t grant = 0;
        uint8_t votes = this->votes_.size();
        uint8_t threshold = this->prs_.size() / 2;
        for(auto vote: this->votes_) {
            if(vote.second) {
                grant++;
            }
        }
        if(grant > threshold) {
            this->BecomeLeader();
        } else if(votes - grant > threshold) {
            this->BecomeFollower(this->term_, NONE);
        }
    }

    bool RaftContext::HandleAppendEntries(eraftpb::Message m) {
        if(m.term() != NONE && m.term() < this->term_) {
            this->SendAppendResponse(m.from(), true, NONE, NONE);
            return false;
        }
        this->electionElapsed_ = 0;
        this->randomElectionTimeout_ = this->electionTimeout_ + RandIntn(this->electionTimeout_);
        this->lead_ = m.from();
        RaftLog* l = this->raftLog_;
        uint64_t lastIndex = l->LastIndex();
        if(m.index() > lastIndex) {
            this->SendAppendResponse(m.from(), true, NONE, lastIndex+1);
            return false;
        }
        if(m.index() >= l->firstIndex_) {
            uint64_t logTerm = l->Term(m.index());
            if(logTerm != m.log_term()) {
                uint64_t index = 0;
                for(uint64_t i = 0; i < l->ToSliceIndex(m.index() + 1); i++) {
                    if(l->entries_[i].term() == logTerm) {
                        index = i;
                    }
                }
                this->SendAppendResponse(m.from(), true, logTerm, index);
                return false;
            }
        }
        uint64_t count = 0;
        for(auto entry: m.entries()) {
            if(entry.index() < l->firstIndex_) {
                continue;
            }
            if(entry.index() <= l->LastIndex()) {
                uint64_t logTerm = l->Term(entry.index());
                if(logTerm != entry.term()) {
                    uint64_t idx = l->ToSliceIndex(entry.index());
                    l->entries_[idx] = entry;
                    l->entries_.erase(l->entries_.begin(), l->entries_.begin() + idx + 1);
                    l->stabled_ = std::min(l->stabled_, entry.index()-1);
                }
            } else {
                uint64_t n = m.entries().size();
                for(uint64_t j = count; j < n; j++) {
                    l->entries_.push_back(m.entries()[j]);
                }
                break;
            }
            count++;
        }
        if(m.commit() > l->commited_) {
            l->commited_ = std::min(m.commit(), m.index() + m.entries().size());
        }
        this->SendAppendResponse(m.from(), false, NONE, l->LastIndex());
    }

    bool RaftContext::HandleAppendEntriesResponse(eraftpb::Message m) {
        // TODO:
    }

    void RaftContext::LeaderCommit() {
        // TODO:
    }

    void RaftContext::HandleHeartbeat(eraftpb::Message m) {
        // TODO:
    }

    void RaftContext::AppendEntries(std::vector<eraftpb::Entry* > entries) {
        // TODO:
    }

    ESoftState* RaftContext::SoftState() {
        // TODO:
    }

    eraftpb::HardState RaftContext::HardState() {
        // TODO:
    }

    void RaftContext::HandleSnapshot(eraftpb::Message m) {
        // TODO:
    }

    void RaftContext::HandleTransferLeader(eraftpb::Message m) {
        // TODO:
    }

    void RaftContext::AddNode(uint64_t id) {
        // TODO:
    }

    void RaftContext::RemoveNode(uint64_t id) {
        // TODO:
    }

} // namespace eraft
