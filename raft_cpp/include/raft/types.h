#pragma once
// types.h — Entry, ApplyMsg, NodeState, RPC message structs
// Corresponds to Go: rpc.go + util.go type definitions

#include <string>
#include <sstream>
#include <vector>

namespace raft {

// ── NodeState ────────────────────────────────────────────────
enum class NodeState : uint8_t {
    Follower  = 0,
    Candidate = 1,
    Leader    = 2
};

inline const char* to_string(NodeState s) {
    switch (s) {
        case NodeState::Follower:  return "Follower";
        case NodeState::Candidate: return "Candidate";
        case NodeState::Leader:    return "Leader";
    }
    return "Unknown";
}

// ── Entry ────────────────────────────────────────────────────
struct Entry {
    int index   = 0;
    int term    = 0;
    std::vector<uint8_t> command;   // empty for dummy / snapshot entries

    std::string to_string() const {
        std::ostringstream oss;
        oss << "Entry{Index:" << index << ",Term:" << term
            << ",CmdLen:" << command.size() << "}";
        return oss.str();
    }
};

// ── ApplyMsg ─────────────────────────────────────────────────
struct ApplyMsg {
    bool command_valid = false;
    std::vector<uint8_t> command;
    int command_index = 0;
    int command_term  = 0;

    bool snapshot_valid = false;
    std::vector<uint8_t> snapshot;
    int snapshot_term  = 0;
    int snapshot_index = 0;

    std::string to_string() const {
        std::ostringstream oss;
        if (command_valid) {
            oss << "Command{Index:" << command_index
                << ",Term:" << command_term << "}";
        } else if (snapshot_valid) {
            oss << "Snapshot{Index:" << snapshot_index
                << ",Term:" << snapshot_term << "}";
        } else {
            oss << "ApplyMsg{empty}";
        }
        return oss.str();
    }
};

// ── RPC: RequestVote ─────────────────────────────────────────
struct RequestVoteRequest {
    int term         = 0;
    int candidate_id = 0;
    int last_log_index = 0;
    int last_log_term  = 0;

    std::string to_string() const {
        std::ostringstream oss;
        oss << "RequestVoteRequest{Term:" << term
            << ",CandidateId:" << candidate_id
            << ",LastLogIndex:" << last_log_index
            << ",LastLogTerm:" << last_log_term << "}";
        return oss.str();
    }
};

struct RequestVoteResponse {
    int  term        = 0;
    bool vote_granted = false;

    std::string to_string() const {
        std::ostringstream oss;
        oss << "RequestVoteResponse{Term:" << term
            << ",VoteGranted:" << (vote_granted ? "true" : "false") << "}";
        return oss.str();
    }
};

// ── RPC: AppendEntries ───────────────────────────────────────
struct AppendEntriesRequest {
    int term         = 0;
    int leader_id    = 0;
    int prev_log_index = 0;
    int prev_log_term  = 0;
    int leader_commit  = 0;
    std::vector<Entry> entries;

    std::string to_string() const {
        std::ostringstream oss;
        oss << "AppendEntriesRequest{Term:" << term
            << ",LeaderId:" << leader_id
            << ",PrevLogIndex:" << prev_log_index
            << ",PrevLogTerm:" << prev_log_term
            << ",LeaderCommit:" << leader_commit
            << ",Entries:[" << entries.size() << "]}";
        return oss.str();
    }
};

struct AppendEntriesResponse {
    int  term          = 0;
    bool success       = false;
    int  conflict_index = 0;
    int  conflict_term  = 0;

    std::string to_string() const {
        std::ostringstream oss;
        oss << "AppendEntriesResponse{Term:" << term
            << ",Success:" << (success ? "true" : "false")
            << ",ConflictIndex:" << conflict_index
            << ",ConflictTerm:" << conflict_term << "}";
        return oss.str();
    }
};

// ── RPC: InstallSnapshot ─────────────────────────────────────
struct InstallSnapshotRequest {
    int term               = 0;
    int leader_id          = 0;
    int last_included_index = 0;
    int last_included_term  = 0;
    std::vector<uint8_t> data;

    std::string to_string() const {
        std::ostringstream oss;
        oss << "InstallSnapshotRequest{Term:" << term
            << ",LeaderId:" << leader_id
            << ",LastIncludedIndex:" << last_included_index
            << ",LastIncludedTerm:" << last_included_term
            << ",DataLen:" << data.size() << "}";
        return oss.str();
    }
};

struct InstallSnapshotResponse {
    int term = 0;

    std::string to_string() const {
        std::ostringstream oss;
        oss << "InstallSnapshotResponse{Term:" << term << "}";
        return oss.str();
    }
};

} // namespace raft
