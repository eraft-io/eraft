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

    std::vector<int64_t> next_idxs_;

    std::vector<int64_t> match_idxs_;

    int64_t leader_id_;

    uint64_t header_beat_tick_;

    uint64_t leader_election_tick_;

public:
    raft();
    ~raft();
};


