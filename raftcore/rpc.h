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
#include "entry.h"

struct request_vote_req
{
    int64_t term;
    int64_t candidate_id;
    int64_t last_log_index;
    int64_t last_log_term;
    request_vote_req(): term(0), candidate_id(0), last_log_index(0), last_log_term(0) {}
};

struct request_vote_resp
{
    int64_t term;
    bool vote_granted;
    request_vote_resp(): term(0), vote_granted(false) {}
};

struct append_entries_req
{
    int64_t term;
    int64_t leader_id;
    int64_t prev_log_index;
    int64_t prev_log_term;
    int64_t leader_commit;
    std::vector<entry> entries;
    append_entries_req(): term(0), leader_id(0), prev_log_index(0), prev_log_term(0), leader_commit(0) {}
};

struct append_entries_resp
{
    int64_t term;
    bool success;
    int64_t conflict_index;
    int64_t conflict_term;
    append_entries_resp(): term(0), success(false), conflict_index(0), conflict_term(0) {}
};

struct install_snapshot_req
{
    int64_t term;
    int64_t leader_id;
    int64_t last_included_index;
    int64_t last_included_term;
    std::vector<uint8_t> data;
    install_snapshot_req(): term(0), leader_id(0), last_included_index(0), last_included_term(0) {}
};

struct install_snapshot_resp
{
    int64_t term;
    install_snapshot_resp(): term(0) {}
};
