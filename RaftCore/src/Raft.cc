#include <RaftCore/Raft.h>
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
    }

    void RaftContext::SendSnapshot(uint64_t to) {
        // TODO:
    }

    bool RaftContext::SendAppend(uint64_t to) {
        // TODO:
        return false;
    }

    void RaftContext::SendAppendResponse(uint64_t to, bool reject, uint64_t term, uint64_t index) {
        // TODO:
    }

    void RaftContext::SendHeartbeat(uint64_t to) {
        // TODO:
    }

    void RaftContext::SendHeartbeatResponse(uint64_t to, bool reject) {
        // TODO:
    }

    void RaftContext::SendRequestVote(uint64_t to, uint64_t index, uint64_t term) {
        // TODO:
    }

    void RaftContext::SendRequestVoteResponse(uint64_t to, bool reject) {
        // TODO:
    }

    void RaftContext::SendTimeoutNow(uint64_t to) {
        // TODO:
    }

    void RaftContext::Tick() {
        // TODO:
    }

    void RaftContext::TickElection() {
        // TODO:
    }

    void RaftContext::TickHeartbeat() {
        // TODO:
    }

    void RaftContext::TickTransfer() {
        // TODO:
    }

    void RaftContext::BecomeFollower(uint64_t term, uint64_t lead) {
        // TODO:
    }

    void RaftContext::BecomeCandidate() {
        // TODO:
    }

    void RaftContext::BecomeLeader() {
        // TODO:
    }

    void RaftContext::StepFollower(eraftpb::Message m) {
        // TODO:
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
