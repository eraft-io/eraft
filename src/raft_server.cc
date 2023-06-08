// MIT License

// Copyright (c) 2023 ERaftGroup

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/**
 * @file raft_server.cc
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-05-21
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "raft_server.h"

#include <bits/stdc++.h>

#include <algorithm>
#include <thread>

#include "rocksdb_storage_impl.h"
#include "util.h"

/**
 * @brief Construct a new Raft Server object
 *
 * @param raft_config
 */
RaftServer::RaftServer(RaftConfig raft_config,
                       LogStore*  log_store,
                       Storage*   store,
                       Network*   net)
    : id_(raft_config.id)
    , role_(NodeRaftRoleEnum::Follower)
    , current_term_(0)
    , voted_for_(-1)
    , commit_idx_(0)
    , last_applied_idx_(0)
    , tick_count_(0)
    , leader_id_(-1)
    , heartbeat_timeout_(1)
    , election_timeout_(0)
    , base_election_timeout_(5)
    , heartbeat_tick_count_(0)
    , election_tick_count_(0)
    , max_entries_per_append_req_(100)
    , tick_interval_(1000)
    , granted_votes_(0)
    , open_auto_apply_(true)
    , election_running_(true) {

  this->log_store_ = log_store;
  this->store_ = store;
  this->net_ = net;
  this->store_->ReadRaftMeta(this, &this->current_term_, &this->voted_for_);
  this->log_store_->ReadMetaState(&this->commit_idx_, &this->last_applied_idx_);
  TraceLog("DEBUG: ",
           " raft server init with current_term_ ",
           current_term_,
           " voted_for_ ",
           voted_for_,
           " commit_idx_ ",
           commit_idx_);
  for (auto n : raft_config.peer_address_map) {
    RaftNode* node = new RaftNode(n.first,
                                  NodeStateEnum::Running,
                                  0,
                                  this->log_store_->LastIndex() + 1,
                                  n.second);
    this->nodes_.push_back(node);
  }
}

EStatus RaftServer::ResetRandomElectionTimeout() {
  // make rand election timeout in (election_timeout, 2 * election_timout)
  auto rand_tick =
      RandomNumber::Between(base_election_timeout_, 2 * base_election_timeout_);
  election_timeout_ = rand_tick;
  election_tick_count_ = 0;
  return EStatus::kOk;
}

RaftServer* RaftServer::RunMainLoop(RaftConfig raft_config,
                                    LogStore*  log_store,
                                    Storage*   store,
                                    Network*   net) {
  RaftServer* svr = new RaftServer(raft_config, log_store, store, net);
  std::thread th(&RaftServer::RunCycle, svr);
  th.detach();
  return svr;
}

/**
 * @brief raft core cycle
 *
 * @return EStatus
 */
EStatus RaftServer::RunCycle() {
  ResetRandomElectionTimeout();
  while (true) {
    TraceLog("DEBUG: ", " election_tick_count_ ", election_tick_count_);
    TraceLog("DEBUG: ", " heartbeat_tick_count_ ", heartbeat_tick_count_);
    TraceLog("DEBUG: ", " node role ", NodeRoleToStr(role_));
    TraceLog("DEBUG: ",
             " commit idx ",
             this->commit_idx_,
             " applied idx ",
             this->last_applied_idx_);
    heartbeat_tick_count_ += 1;
    election_tick_count_ += 1;
    if (heartbeat_tick_count_ == heartbeat_timeout_) {
      if (this->role_ == NodeRaftRoleEnum::Leader) {
        TraceLog("DEBUG: ", "heartbeat timeout");
        this->SendHeartBeat();
        heartbeat_tick_count_ = 0;
      }
    }
    if (election_tick_count_ >= election_timeout_ && election_running_) {
      if(this->role_ == NodeRaftRoleEnum::Follower){
        TraceLog("DEBUG: ", "start pre election in term", current_term_);
        this->BecomePreCandidate();
        this->ElectionStart(true);
      }else if (this->role_ == NodeRaftRoleEnum::PreCandidate){
        if(this->granted_votes_ > (this->nodes_.size() / 2)){
          this->BecomeCandidate();
          this->ElectionStart(false);
        }else{
          this->BecomeFollower();
        }
        // TraceLog("DEBUG: ", "start election in term", current_term_);
        // this->BecomeCandidate();
        // this->current_term_ += 1;
        // this->ElectionStart(true);
      }else if(this->role_ == NodeRaftRoleEnum::Candidate){
        if(this->granted_votes_ > (this->nodes_.size() / 2)){
          this->BecomeLeader();
        }else{
          this->BecomeFollower();
        }
      }
      ResetRandomElectionTimeout();
    }
    if (open_auto_apply_) {
      this->ApplyEntries();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::SendAppendEntries() {

  // only leader can set append entries
  if (this->role_ != NodeRaftRoleEnum::Leader) {
    return EStatus::kNotSupport;
  }

  for (auto& node : this->nodes_) {
    if (node->id == this->id_ || node->node_state == NodeStateEnum::Down) {
      continue;
    }

    auto prev_log_index = node->next_log_index - 1;

    if (prev_log_index < this->log_store_->FirstIndex()) {
      TraceLog("send snapshot to node: ", node->id);
    } else {
      auto prev_log_entry = this->log_store_->Get(prev_log_index);
      auto copy_cnt = this->log_store_->LastIndex() - prev_log_index;
      if (copy_cnt > this->max_entries_per_append_req_) {
        copy_cnt = this->max_entries_per_append_req_;
      }

      eraftkv::AppendEntriesReq* append_req = new eraftkv::AppendEntriesReq();
      append_req->set_is_heartbeat(false);
      if (copy_cnt > 0) {
        auto etys_to_be_send = this->log_store_->Gets(
            prev_log_index + 1, prev_log_index + copy_cnt);
        for (auto ety : etys_to_be_send) {
          eraftkv::Entry* new_ety = append_req->add_entries();
          new_ety->set_id(ety->id());
          new_ety->set_e_type(ety->e_type());
          new_ety->set_term(ety->term());
          new_ety->set_allocated_data(ety->mutable_data());
        }
      }
      append_req->set_term(this->current_term_);
      append_req->set_leader_id(this->id_);
      append_req->set_prev_log_index(prev_log_index);
      append_req->set_prev_log_term(prev_log_entry->term());
      append_req->set_leader_commit(this->commit_idx_);

      this->net_->SendAppendEntries(this, node, append_req);
      delete prev_log_entry;
      delete append_req;
    }
  }

  return EStatus::kOk;
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::ApplyEntries() {
  this->store_->ApplyLog(this, 0, 0);
  return EStatus::kOk;
}

bool RaftServer::IsUpToDate(int64_t last_idx, int64_t term) {
  return last_idx >= this->log_store_->LastIndex() &&
         term >= this->log_store_->GetLastEty()->term();
}

/**
 * @brief
 *
 * @param from_node
 * @param req
 * @param resp
 * @return EStatus
 */
EStatus RaftServer::HandleRequestVoteReq(RaftNode* from_node,
                                         const eraftkv::RequestVoteReq* req,
                                         eraftkv::RequestVoteResp*      resp) {
  TraceLog("DEBUG: handle vote req ",
           " term ",
           req->term(),
           " candidtate_id ",
           req->candidtate_id(),
           " last_log_idx ",
           req->last_log_idx(),
           " last_log_term ",
           req->last_log_term());

  TraceLog("DEBUG:  handle vote req",
           " current_term_ ",
           current_term_,
           " leader_id_ ",
           leader_id_);
  resp->set_term(current_term_);
  resp->set_prevote(req->prevote());
  if(this->current_term_ > req->term()){
    resp->set_vote_granted(false);
    return EStatus::kOk;
  }
  // resp->set_leader_id(leader_id_);

  bool can_vote = (this->voted_for_ == req->candidtate_id()) ||
                  this->voted_for_ == -1 && this->leader_id_ == -1 ||
                  req->term() > this->current_term_;

  if (can_vote && this->IsUpToDate(req->last_log_idx(), req->last_log_term())) {
    resp->set_vote_granted(true);
  } else {
    resp->set_vote_granted(false);
    this->store_->SaveRaftMeta(this, this->current_term_, this->voted_for_);
    return EStatus::kOk;
  }
  if(!req->prevote()){
    TraceLog("DEBUG: peer ", this->id_, " vote ", req->candidtate_id());
    this->voted_for_ = req->candidtate_id();

    ResetRandomElectionTimeout();
    election_tick_count_ = 0;
  }
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::SendHeartBeat() {
  for (auto node : this->nodes_) {
    if (node->id == this->id_ || node->node_state == NodeStateEnum::Down) {
      continue;
    }

    eraftkv::AppendEntriesReq* append_req = new eraftkv::AppendEntriesReq();
    append_req->set_is_heartbeat(true);
    append_req->set_leader_id(this->id_);
    append_req->set_term(this->current_term_);
    append_req->set_leader_commit(this->commit_idx_);

    this->net_->SendAppendEntries(this, node, append_req);
  }

  return EStatus::kOk;
}

/**
 * @brief
 *
 * @param from_node
 * @param resp
 * @return EStatus
 */
EStatus RaftServer::HandleRequestVoteResp(RaftNode* from_node,
                                          const eraftkv::RequestVoteReq* req,
                                          eraftkv::RequestVoteResp*      resp) {
  if (resp != nullptr) {
    TraceLog("DEBUG: ",
             " send request vote revice resp: ",
             " term ",
             resp->term(),
             " vote_granted ",
             resp->vote_granted(),
             " leader_id",
             resp->leader_id(),
             " from node ",
             from_node->address);
    if(this->role_ == NodeRaftRoleEnum::PreCandidate && 
        req->term() == this->current_term_+1 && resp->prevote()){
      if(resp->vote_granted()){
        this->granted_votes_ += 1;
        if(this->granted_votes_ > (this->nodes_.size() / 2)){
          TraceLog("DEBUG: ",
                   " node ",
                   this->id_,
                   " get majority prevotes in term ",
                   this->current_term_);
          this->election_tick_count_ = this->election_timeout_;
          // this->BecomeCandidate();
          // this->granted_votes_ = 0;
        }
      }
    }
    if (this->current_term_ == req->term() &&
        this->role_ == NodeRaftRoleEnum::Candidate && !resp->prevote()) {
      if (resp->vote_granted()) {
        this->granted_votes_ += 1;
        if (this->granted_votes_ > (this->nodes_.size() / 2)) {
          TraceLog("DEBUG: ",
                   " node ",
                   this->id_,
                   " get majority votes in term ",
                   this->current_term_);
          this->BecomeLeader();
          this->SendHeartBeat();
          this->SendAppendEntries();
          this->granted_votes_ = 0;
        }
      } else {
        if (resp->leader_id() != -1) {
          this->leader_id_ = resp->leader_id();
          this->current_term_ = resp->term();
          this->BecomeFollower();
          this->voted_for_ = -1;
          this->store_->SaveRaftMeta(
              this, this->current_term_, this->voted_for_);
        }
        if (resp->term() >= this->current_term_) {
          this->BecomeFollower();
          this->voted_for_ = -1;
          this->store_->SaveRaftMeta(
              this, this->current_term_, this->voted_for_);
        }
      }
    }
  }
  return EStatus::kOk;
}

EStatus RaftServer::Propose(std::string payload,
                            int64_t*    new_log_index,
                            int64_t*    new_log_term,
                            bool*       is_success) {
  if (this->role_ != NodeRaftRoleEnum::Leader) {
    *new_log_index = -1;
    *new_log_term = -1;
    *is_success = false;
    return EStatus::kOk;
  }
  // TODO: reject when snapshoting
  eraftkv::Entry* new_ety = new eraftkv::Entry();
  new_ety->set_data(payload);
  new_ety->set_id(this->log_store_->LastIndex() + 1);
  new_ety->set_term(this->current_term_);
  new_ety->set_e_type(eraftkv::EntryType::Normal);

  this->log_store_->Append(new_ety);

  for (auto node : this->nodes_) {
    if (node->id == this->id_) {
      node->match_log_index = new_ety->id();
      node->next_log_index = node->match_log_index + 1;
    }
  }
  SendAppendEntries();
  *new_log_index = new_ety->id();
  *new_log_term = new_ety->term();
  *is_success = true;
  delete new_ety;
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @param from_node
 * @param req
 * @param resp
 * @return EStatus
 */
EStatus RaftServer::HandleAppendEntriesReq(RaftNode* from_node,
                                           const eraftkv::AppendEntriesReq* req,
                                           eraftkv::AppendEntriesResp* resp) {
  ResetRandomElectionTimeout();
  election_tick_count_ = 0;

  if (req->is_heartbeat()) {
    TraceLog("DEBUG: recv heart beat");
    this->AdvanceCommitIndexForFollower(req->leader_commit());
    resp->set_success(true);
    this->leader_id_ = req->leader_id();
    this->current_term_ = req->term();
    this->BecomeFollower();
    this->store_->SaveRaftMeta(this, this->current_term_, this->voted_for_);
    return EStatus::kOk;
  }

  resp->set_term(this->current_term_);
  if (req->term() < this->current_term_ &&
      this->log_store_->LastIndex() > req->prev_log_index()) {
    resp->set_success(false);
    return EStatus::kOk;
  }

  if (req->term() > this->current_term_) {
    this->current_term_ = req->term();
    this->voted_for_ = -1;
    this->store_->SaveRaftMeta(this, this->current_term_, this->voted_for_);
  }

  this->BecomeFollower();
  this->leader_id_ = req->leader_id();

  if (req->prev_log_index() < this->log_store_->FirstIndex()) {
    resp->set_conflict_index(this->log_store_->FirstIndex());
    resp->set_conflict_term(0);
    resp->set_success(false);
    return EStatus::kOk;
  }

  if (!this->MatchLog(req->prev_log_term(), req->prev_log_index())) {
    resp->set_success(false);
    if (this->log_store_->LastIndex() < req->prev_log_index()) {
      resp->set_conflict_index(this->log_store_->LastIndex());
      resp->set_conflict_term(this->log_store_->GetLastEty()->term());
    } else {
      // set term
      resp->set_conflict_term(
          this->log_store_->Get(req->prev_log_index())->term());
      // find conflict index
      auto index = req->prev_log_index();
      while (index >= this->commit_idx_ &&
             this->log_store_->Get(index)->term() == resp->conflict_term()) {
        index -= 1;
      }
      resp->set_conflict_index(index);
    }
  } else {
    for (auto ety : req->entries()) {
      this->log_store_->Append(&ety);
      this->log_store_->PersisLogMetaState(this->commit_idx_,
                                           this->last_applied_idx_);
    }
    this->AdvanceCommitIndexForFollower(req->leader_commit());
    resp->set_success(true);
  }

  return EStatus::kOk;
}

/**
 * @brief
 *
 * @param from_node
 * @param resp
 * @return EStatus
 */
EStatus RaftServer::HandleAppendEntriesResp(RaftNode* from_node,
                                            eraftkv::AppendEntriesReq*  req,
                                            eraftkv::AppendEntriesResp* resp) {
  if (role_ == NodeRaftRoleEnum::Leader) {
    if (resp != nullptr) {
      if (resp->success()) {
        for (auto node : this->nodes_) {
          if (node->node_state == NodeStateEnum::Down) {
            continue;
          }
          if (from_node->id == node->id) {
            node->match_log_index =
                req->prev_log_index() + req->entries().size();
            node->next_log_index = node->match_log_index + 1;
            this->AdvanceCommitIndexForLeader();
          }
        }
      } else {
        if (resp->term() > req->term()) {
          this->BecomeFollower();
          this->current_term_ = resp->term();
          this->voted_for_ = -1;
          this->store_->SaveRaftMeta(
              this, this->current_term_, this->voted_for_);
        } else {
          if (resp->conflict_term() == 0) {
            for (auto node : this->nodes_) {
              if (from_node->id == node->id) {
                node->next_log_index = resp->conflict_index();
              }
            }
          } else {
            for (auto node : this->nodes_) {
              if (node->node_state == NodeStateEnum::Down) {
                continue;
              }
              if (from_node->id == node->id) {
                node->next_log_index = resp->conflict_index();
                for (int64_t i = resp->conflict_index();
                     i >= this->log_store_->FirstIndex();
                     i--) {
                  if (this->log_store_->Get(i)->term() ==
                      resp->conflict_term()) {
                    node->next_log_index = i + 1;
                    break;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return EStatus::kOk;
}


/**
 * @brief
 *
 * @param from_node
 * @param req
 * @param resp
 * @return EStatus
 */
EStatus RaftServer::HandleSnapshotReq(RaftNode*              from_node,
                                      eraftkv::SnapshotReq*  req,
                                      eraftkv::SnapshotResp* resp) {
  return EStatus::kOk;
}

/**
 * @brief the log entry with index match term
 *
 * @param term
 * @param index
 * @return true
 * @return false
 */
bool RaftServer::MatchLog(int64_t term, int64_t index) {
  return (index <= this->log_store_->LastIndex() &&
          index >= this->log_store_->FirstIndex() &&
          this->log_store_->Get(index)->term() == term);
}


EStatus RaftServer::AdvanceCommitIndexForLeader() {
  std::vector<int64_t> match_idxs;
  for (auto node : this->nodes_) {
    if (node->node_state == NodeStateEnum::Down) {
      continue;
    }
    match_idxs.push_back(node->match_log_index);
  }
  sort(match_idxs.begin(), match_idxs.end());
  int64_t new_commit_index = match_idxs[match_idxs.size() / 2];
  if (new_commit_index > this->commit_idx_) {
    if (this->MatchLog(this->current_term_, new_commit_index)) {
      this->commit_idx_ = new_commit_index;
      this->log_store_->PersisLogMetaState(this->commit_idx_,
                                           this->last_applied_idx_);
    }
  }
  return EStatus::kOk;
}

EStatus RaftServer::AdvanceCommitIndexForFollower(int64_t leader_commit) {

  int64_t new_commit_index =
      std::min(leader_commit, this->log_store_->GetLastEty()->id());
  if (new_commit_index > this->commit_idx_) {
    this->commit_idx_ = new_commit_index;
    this->log_store_->PersisLogMetaState(this->commit_idx_,
                                         this->last_applied_idx_);
  }
  return EStatus::kOk;
}


/**
 * @brief
 *
 * @param from_node
 * @param resp
 * @return EStatus
 */
EStatus RaftServer::HandleSnapshotResp(RaftNode*              from_node,
                                       eraftkv::SnapshotResp* resp) {
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @param from_node
 * @param ety
 * @param ety_index
 * @return EStatus
 */
EStatus RaftServer::HandleApplyConfigChange(RaftNode*       from_node,
                                            eraftkv::Entry* ety,
                                            int64_t         ety_index) {
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @param payload
 * @param new_log_index
 * @param new_log_term
 * @param is_success
 * @return EStatus
 */
EStatus RaftServer::ProposeConfChange(std::string payload,
                                      int64_t*    new_log_index,
                                      int64_t*    new_log_term,
                                      bool*       is_success) {
  if (this->role_ != NodeRaftRoleEnum::Leader) {
    *new_log_index = -1;
    *new_log_term = -1;
    *is_success = false;
    return EStatus::kOk;
  }

  eraftkv::Entry* new_ety = new eraftkv::Entry();
  new_ety->set_data(payload);
  new_ety->set_id(this->log_store_->LastIndex() + 1);
  new_ety->set_term(this->current_term_);
  new_ety->set_e_type(eraftkv::EntryType::ConfChange);

  this->log_store_->Append(new_ety);

  for (auto node : this->nodes_) {
    if (node->node_state == NodeStateEnum::Down) {
      continue;
    }
    if (node->id == this->id_) {
      node->match_log_index = new_ety->id();
      node->next_log_index = node->match_log_index + 1;
    }
  }

  SendAppendEntries();

  *new_log_index = new_ety->id();
  *new_log_term = new_ety->term();
  *is_success = true;

  delete new_ety;
  return EStatus::kOk;
}


/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::BecomeLeader() {
  this->role_ = NodeRaftRoleEnum::Leader;
  heartbeat_tick_count_ = 0;
  election_running_ = false;
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::BecomeFollower() {
  this->role_ = NodeRaftRoleEnum::Follower;
  ResetRandomElectionTimeout();
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::BecomeCandidate() {
  if (this->role_ == NodeRaftRoleEnum::Candidate) {
    return EStatus::kOk;
  }
  this->role_ = NodeRaftRoleEnum::Candidate;
  this->current_term_ += 1;
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::BecomePreCandidate() {
  if(this->role_ == NodeRaftRoleEnum::Leader || this->role_ == NodeRaftRoleEnum::Candidate){
    return EStatus::kError;
  }
  this->role_ = NodeRaftRoleEnum::PreCandidate;
  this->granted_votes_ = 0;
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @param is_prevote
 * @return EStatus
 */
EStatus RaftServer::ElectionStart(bool is_prevote) {
  TraceLog("DEBUG: ",
           this->id_,
           " start a new election in term ",
           this->current_term_);
  this->granted_votes_ = 1;
  this->leader_id_ = -1;
  this->voted_for_ = this->id_;
  eraftkv::RequestVoteReq* vote_req = new eraftkv::RequestVoteReq();
  if(is_prevote){
    vote_req->set_term(this->current_term_+1);
    vote_req->set_prevote(true);
  }else{
    this->current_term_ += 1;
    vote_req->set_term(this->current_term_);
  }
  // vote_req->set_term(this->current_term_);
  vote_req->set_candidtate_id(this->id_);
  vote_req->set_last_log_idx(this->log_store_->LastIndex());
  vote_req->set_last_log_term(this->log_store_->GetLastEty()->term());
  this->store_->SaveRaftMeta(this, this->current_term_, this->voted_for_);

  for (auto node : this->nodes_) {
    if (node->id == this->id_ || this->role_ == NodeRaftRoleEnum::Leader ||
        node->node_state == NodeStateEnum::Down) {
      continue;
    }

    TraceLog("DEBUG: ",
             "send request vote to ",
             node->address,
             " term ",
             this->current_term_,
             " candidtate_id ",
             this->id_,
             " last_log_idx ",
             this->log_store_->LastIndex(),
             " last_log_term ",
             this->log_store_->GetLastEty()->term());
    this->net_->SendRequestVote(this, node, vote_req);
  }

  return EStatus::kOk;
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::BeginSnapshot() {
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::EndSnapshot() {
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool RaftServer::SnapshotRunning() {
  return false;
}

/**
 * @brief Get the Last Applied Entry object
 *
 * @return Entry*
 */
eraftkv::Entry* RaftServer::GetLastAppliedEntry() {
  return nullptr;
}

/**
 * @brief Get the First Entry Idx object
 *
 * @return int64_t
 */
int64_t RaftServer::GetFirstEntryIdx() {
  return 0;
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::RestoreSnapshotAfterRestart() {
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @param last_included_term
 * @param last_included_index
 * @return EStatus
 */
EStatus RaftServer::BeginLoadSnapshot(int64_t last_included_term,
                                      int64_t last_included_index) {
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::EndLoadSnapshot() {
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::ProposeReadReq() {
  return EStatus::kOk;
}

/**
 * @brief Get the Logs Count Can Snapshot object
 *
 * @return int64_t
 */
int64_t RaftServer::GetLogsCountCanSnapshot() {
  return 0;
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::RestoreLog() {
  return EStatus::kOk;
}
