#ifndef ERAFT_RAFT_H
#define ERAFT_RAFT_H

#include <eraftio/eraftpb.pb.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <RaftCore/Log.h>

namespace eraft
{

class ESoftState
{
public:

    uint64_t lead;

    NodeState& raftState;

    bool Equal(ESoftState* b);
};

struct Config {
    
    // ID is the identity of the local raft. ID cannot be 0.
    uint64_t id;

	// peers contains the IDs of all nodes (including self) in the raft cluster. It
	// should only be set when starting a new raft cluster. Restarting raft from
	// previous configuration will panic if peers is set. peer is private and only
	// used for testing right now.
    std::vector<uint64_t> peers;

	// ElectionTick is the number of Node.Tick invocations that must pass between
	// elections. That is, if a follower does not receive any message from the
	// leader of current term before ElectionTick has elapsed, it will become
	// candidate and start an election. ElectionTick must be greater than
	// HeartbeatTick. We suggest ElectionTick = 10 * HeartbeatTick to avoid
	// unnecessary leader switching.
    uint64_t electionTick;

	// HeartbeatTick is the number of Node.Tick invocations that must pass between
	// heartbeats. That is, a leader sends heartbeat messages to maintain its
	// leadership every HeartbeatTick ticks.
    uint64_t heartbeatTick;

	// Storage is the storage for raft. raft generates entries and states to be
	// stored in storage. raft reads the persisted entries and states out of
	// Storage when it needs. raft reads out the previous state and configuration
	// out of storage when restarting.
    StorageInterface *storage;

	// Applied is the last applied index. It should only be set when restarting
	// raft. raft will not return entries to the application smaller or equal to
	// Applied. If Applied is unset when restarting, raft might return previous
	// applied entries. This is a very application dependent configuration.
    uint64_t applied;

    // validate config
    bool Validate();
};

struct Progress {
    
    uint64_t match;

    uint64_t next;

};

class RaftContext {

public:

    RaftContext(Config *c);

    // Step the entrance of handle message, see `MessageType`
    // on `eraftpb.proto` for what msgs should be handled
    void Step(eraftpb::Message m);

private:

    void SendSnapshot(uint64_t to);

    bool SendAppend(uint64_t to);

    void SendAppendResponse(uint64_t to, bool reject, uint64_t term, uint64_t index);

    void SendHeartbeat(uint64_t to);

    void SendHeartbeatResponse(uint64_t to, bool reject);

    void SendRequestVote(uint64_t to, uint64_t index, uint64_t term);

    void SendRequestVoteResponse(uint64_t to, bool reject);

    void SendTimeoutNow(uint64_t to);

    void Tick();

    void TickElection();

    void TickHeartbeat();

    void TickTransfer();

    void BecomeFollower(uint64_t term, uint64_t lead);

    void BecomeCandidate();

    void BecomeLeader();

    void StepFollower(eraftpb::Message m);

    void StepCandidate(eraftpb::Message m);

    void StepLeader(eraftpb::Message m);

    void DoElection();

    void BcastHeartbeat();

    void BcastAppend();

    void HandleRequestVote(eraftpb::Message m);

    void HandleRequestVoteResponse(eraftpb::Message m);

    void HandleAppendEntries(eraftpb::Message m);

    void HandleAppendEntriesResponse(eraftpb::Message m);

    void LeaderCommit();

    void HandleHeartbeat(eraftpb::Message m);

    void AppendEntries(std::vector<eraftpb::Entry* > entries);

    ESoftState* SoftState();

    eraftpb::HardState HardState();

    void HandleSnapshot(eraftpb::Message m);

    void HandleTransferLeader(eraftpb::Message m);

    void AddNode(uint64_t id);

    void RemoveNode(uint64_t id);

    uint64_t id_;

    uint64_t term_;

    uint64_t vote_;

    RaftLog *raftLog_;

    std::map<uint64_t, Progress *> prs_;

    NodeState state_;

    std::map<uint64_t, bool> votes_;

    std::vector<eraftpb::Message> msg_;

    uint64_t lead_;

    uint64_t heartbeatTimeout_;

    uint64_t electionTimeout_;

    uint64_t randomElectionTimeout_;

	// number of ticks since it reached last heartbeatTimeout.
	// only leader keeps heartbeatElapsed.
    uint64_t heartbeatElapsed_;

	// number of ticks since it reached last electionTimeout
    uint64_t electionElapsed_;

    uint64_t transferElapsed_;

    uint64_t leadTransferee_;

    uint64_t pendingConfIndex_;
};


} // namespace eraft


#endif