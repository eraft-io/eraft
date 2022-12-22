#include "raft.h"

raft_stack::raft_stack() {

}

raft_stack::~raft_stack() {

}

/// @brief 
/// @return current term, is leader
/// 
std::pair<uint64_t, bool> raft_stack::get_state() {
    return std::make_pair(this->cur_term_, this->role_ == RAFT_ROLE::LEADER);
}

/// @brief 

void raft_stack::persist_state() {}

/// @brief 
/// @param data 
void raft_stack::read_persist(std::vector<uint8_t> data) {}

/// @brief 
/// @return 
std::vector<uint8_t> raft_stack::encode_state() {}

/// @brief 
/// @param last_included_term 
/// @param last_included_index 
/// @param snapshot 
/// @return 
bool raft_stack::cond_install_snapshot(int64_t last_included_term, uint64_t last_included_index, std::vector<uint8_t> snapshot) {}

/// @brief 
/// @param index 
/// @param snapshot_data 
void raft_stack::snapshot(int64_t index, std::vector<uint8_t> snapshot_data) {}

/// @brief 
/// @param req 
/// @param resp 
void raft_stack::request_vote(request_vote_req& req, request_vote_resp& resp) {}

/// @brief 
/// @param req 
/// @param resp 
void raft_stack::append_entries(append_entries_req& req, append_entries_resp& resp) {}

/// @brief 
/// @param req 
/// @param resp 
void raft_stack::install_snapshot(install_snapshot_req& req, install_snapshot_resp& resp) {}

/// @brief 
void raft_stack::start_election() {}

/// @brief 
/// @param is_heart_beat 
void raft_stack::broadcast_heartbeat(bool is_heart_beat) {}

/// @brief 
/// @param peer 
void raft_stack::replicate_one_round(int64_t peer) {}

///
request_vote_req raft_stack::make_request_vote_req() {}

/// @brief 
/// @param prev_log_index 
/// @return 
append_entries_req raft_stack::make_append_entries_req(int64_t prev_log_index) {}

/// @brief 
/// @param peer 
/// @param req 
/// @param resp 
void raft_stack::handle_append_entries_response(uint16_t peer, append_entries_req& req, append_entries_resp& resp) {}

/// @brief 
/// @return 
install_snapshot_resp raft_stack::make_install_snapshot_resp() {}

/// @brief 
/// @param peer 
/// @param req 
/// @param resp 
void raft_stack::handle_install_snapshot_response(uint16_t peer, install_snapshot_req& req, install_snapshot_resp& resp) {}

/// @brief 
/// @param role 
void raft_stack::change_state(RAFT_ROLE role) {}

///
void raft_stack::advance_commit_index_for_leader() {}

/// @brief 
/// @param leader_commit 
void raft_stack::advance_commit_index_for_follower(int64_t leader_commit) {}

///
entry raft_stack::get_last_log() {}

///
entry raft_stack::get_first_log() {}

/// @brief 
/// @param term 
/// @param index 
/// @return 
bool raft_stack::is_log_up_to_date(int64_t term, int64_t index) {}

/// @brief 
/// @param term 
/// @param index 
/// @return 
bool raft_stack::match_log(int64_t term, int64_t index) {}

/// @brief 
/// @param context 
/// @return 
entry raft_stack::append_new_entry(std::vector<uint8_t> context) {}

/// @brief 
/// @param peer 
/// @return 
bool raft_stack::need_replicating(uint16_t peer) {}

///
bool raft_stack::has_log_in_current_term() {}

/// @brief 
/// @param context 
/// @return 
std::pair<int64_t, int64_t> raft_stack::propose(std::vector<uint8_t> context, bool& is_leader) {
    return std::make_pair<int64_t, int64_t>(0, 0);
}

///
uint16_t raft_stack::my_id() {}

///
void raft_stack::run_ticker() {}

///
void raft_stack::applier() {}

/// @brief 
/// @param peer 
void raft_stack::replicator(uint16_t peer) {}
