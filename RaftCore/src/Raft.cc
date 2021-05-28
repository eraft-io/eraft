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
        }
    }

    void RaftContext::StepCandidate(eraftpb::Message m) {
        // TODO:
    }

    void RaftContext::StepLeader(eraftpb::Message m) {
        // TODO:
    }

    void RaftContext::DoElection() {
        // TODO:
    }

    void RaftContext::BcastHeartbeat() {
        // TODO:
    }

    void RaftContext::BcastAppend() {
        // TODO:
    }

    void RaftContext::HandleRequestVote(eraftpb::Message m) {
        // TODO:
    }

    void RaftContext::HandleRequestVoteResponse(eraftpb::Message m) {
        // TODO:
    }

    void RaftContext::HandleAppendEntries(eraftpb::Message m) {
        // TODO:
    }

    void RaftContext::HandleAppendEntriesResponse(eraftpb::Message m) {
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
