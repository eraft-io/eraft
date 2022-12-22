// Copyright 2022 The uhp-sql Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include <stdint.h>
#include <vector>
#include "entry.h"
#include "rpc.h"

enum class RAFT_ROLE {
    FOLLOWER,
    CANDIDATE,
    LEADER
};

class raft_stack
{
private:

    uint16_t me_id_;

    RAFT_ROLE role_;

    uint64_t cur_term_;

    int64_t voted_for_;

    int64_t granted_votes_;

    int64_t commit_idx_;

    int64_t last_applied_;
    
    std::vector<entry> logs_;

    std::vector<int64_t> next_idxs_;

    std::vector<int64_t> match_idxs_;

    uint16_t leader_id_;

    uint64_t header_beat_tick_;

    uint64_t leader_election_tick_;

public:
    
    std::pair<uint64_t, bool> get_state();
    
    void persist_state();

    void read_persist(std::vector<uint8_t> data);

    std::vector<uint8_t> encode_state();

    bool cond_install_snapshot(int64_t last_included_term, uint64_t last_included_index, std::vector<uint8_t> snapshot);

    void snapshot(int64_t index, std::vector<uint8_t> snapshot_data);

    void request_vote(request_vote_req& req, request_vote_resp& resp);
    
    void append_entries(append_entries_req& req, append_entries_resp& resp);

    void install_snapshot(install_snapshot_req& req, install_snapshot_resp& resp);

    void start_election();

    void broadcast_heartbeat(bool is_heart_beat);

    void replicate_one_round(int64_t peer);

    request_vote_req make_request_vote_req();

    append_entries_req make_append_entries_req(int64_t prev_log_index);

    void handle_append_entries_response(uint16_t peer, append_entries_req& req, append_entries_resp& resp);

    install_snapshot_resp make_install_snapshot_resp();

    void handle_install_snapshot_response(uint16_t peer, install_snapshot_req& req, install_snapshot_resp& resp);

    void change_state(RAFT_ROLE role);

    void advance_commit_index_for_leader();

    void advance_commit_index_for_follower(int64_t leader_commit);

    entry get_last_log();

    entry get_first_log();
    
    bool is_log_up_to_date(int64_t term, int64_t index);

    bool match_log(int64_t term, int64_t index);

    entry append_new_entry(std::vector<uint8_t> context);

    bool need_replicating(uint16_t peer);

    bool has_log_in_current_term();

    std::pair<int64_t, int64_t> propose(std::vector<uint8_t> context, bool& is_leader);

    uint16_t my_id();

    void run_ticker();

    void applier();

    void replicator(uint16_t peer);

    raft_stack();

    ~raft_stack();

};
