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
#include "eraft/raft_server.h"

#include <bits/stdc++.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>

#include "eraft/util.h"

/**
 * @brief construct a new raftserver
 *
 * @param raft_config
 * @param log_store
 * @param store
 * @param net
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
    , heartbeat_timeout_(2)
    , election_timeout_(0)
    , base_election_timeout_(10)
    , heartbeat_tick_count_(0)
    , election_tick_count_(0)
    , max_entries_per_append_req_(100)
    , tick_interval_(100)
    , granted_votes_(0)
    , snap_threshold_log_count_(100000)
    , open_auto_apply_(true)
    , is_snapshoting_(false)
    , snap_db_path_(raft_config.snap_path)
    , election_running_(true)
    , ready_to_apply_(false) {
  this->log_store_ = log_store;
  this->store_ = store;
  this->net_ = net;
  this->store_->ReadRaftMeta(this, &this->current_term_, &this->voted_for_);
  this->log_store_->ReadMetaState(&this->commit_idx_, &this->last_applied_idx_);
  SPDLOG_INFO(
      " raft server init with current_term {}  voted_for {}  commit_idx {}",
      current_term_,
      voted_for_,
      commit_idx_);
  for (auto n : raft_config.peer_address_map) {
    RaftNode* node = new RaftNode(n.first,
                                  NodeStateEnum::Running,
                                  this->log_store_->LastIndex() + 1,
                                  0,
                                  n.second);
    this->nodes_.push_back(node);
  }
}

/**
 * @brief destroy the raftserver
 *
 */
RaftServer::~RaftServer() {
  delete this->log_store_;
  delete this->net_;
  delete this->store_;
}

/**
 * @brief generate random election timeout for election
 *
 * @return EStatus
 */
EStatus RaftServer::ResetRandomElectionTimeout() {
  // make rand election timeout in (election_timeout, 2 * election_timout)
  auto rand_tick =
      RandomNumber::Between(base_election_timeout_, 2 * base_election_timeout_);
  election_timeout_ = rand_tick;
  election_tick_count_ = 0;
  return EStatus::kOk;
}

/**
 * @brief run server mainloop
 *
 * @param raft_config
 * @param log_store
 * @param store
 * @param net
 * @return RaftServer*
 */
RaftServer* RaftServer::RunMainLoop(
    RaftConfig                               raft_config,
    LogStore*                                log_store,
    Storage*                                 store,
    Network*                                 net,
    std::map<int, std::condition_variable*>* response_ready_singals,
    std::mutex*                              response_ready_mutex,
    bool*                                    is_ok_to_response) {
  RaftServer* svr = new RaftServer(raft_config, log_store, store, net);
  svr->response_ready_singals_ = response_ready_singals;
  svr->response_ready_mutex_ = response_ready_mutex;
  svr->is_ok_to_response_ = is_ok_to_response;
  std::thread cycleThread(&RaftServer::RunCycle, svr);
  cycleThread.detach();
  std::thread applyThread(&RaftServer::RunApply, svr);
  applyThread.detach();
  return svr;
}

/**
 * @brief run apply
 *
 */
void RaftServer::RunApply() {
  while (true) {
    {
      std::unique_lock<std::mutex> lock(apply_ready_mtx_);
      apply_ready_cv_.wait(lock, [this] { return this->ready_to_apply_; });
    }
    if (open_auto_apply_) {
      this->ApplyEntries();
    }
    this->ready_to_apply_ = false;
  }
}

/**
 * @brief raft core cycle
 *
 * @return EStatus
 */
EStatus RaftServer::RunCycle() {
  this->ResetRandomElectionTimeout();
  while (true) {
    SPDLOG_INFO("heartbeat_tick_count_  " +
                std::to_string(heartbeat_tick_count_));
    SPDLOG_INFO("election_tick_count_  " +
                std::to_string(election_tick_count_));
    SPDLOG_INFO("election_timeout_  " + std::to_string(election_timeout_));
    SPDLOG_INFO("node role " + NodeRoleToStr(role_));
    SPDLOG_INFO("commit idx {} applied idx {}",
                this->commit_idx_,
                this->last_applied_idx_);
    heartbeat_tick_count_ += 1;
    election_tick_count_ += 1;
    if (heartbeat_tick_count_ == heartbeat_timeout_ &&
        this->role_ == NodeRaftRoleEnum::Leader) {
      SPDLOG_INFO("heartbeat timeout");
      this->SendHeartBeat();
      this->ResetRandomElectionTimeout();
      heartbeat_tick_count_ = 0;
    }
    if (election_tick_count_ >= election_timeout_) {
      switch (this->role_) {
        case NodeRaftRoleEnum::Follower: {
          SPDLOG_INFO("start pre election in term {} ", current_term_);
          this->BecomePreCandidate();
          this->ElectionStart(true);
          break;
        }
        case NodeRaftRoleEnum::PreCandidate: {
          if (this->granted_votes_ > (this->nodes_.size() / 2)) {
            this->BecomeCandidate();
            this->ElectionStart(false);
          } else {
            this->BecomeFollower();
          }
          break;
        }
        case NodeRaftRoleEnum::Candidate: {
          if (this->granted_votes_ > (this->nodes_.size() / 2)) {
            this->BecomeLeader();
          } else {
            this->BecomeFollower();
          }
          break;
        }
        default:
          break;
      }
      ResetRandomElectionTimeout();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(tick_interval_));
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

  for (auto node : this->nodes_) {
    if (node->id == this->id_ ||
        node->node_state == NodeStateEnum::LostConnection) {
      continue;
    }

    auto prev_log_index = node->next_log_index - 1;

    SPDLOG_INFO("node prev_log_index {} node id {}", prev_log_index, node->id);
    SPDLOG_INFO("current node fist log index {}",
                this->log_store_->FirstIndex());

    if (prev_log_index < this->log_store_->FirstIndex()) {
      auto new_first_log_ent = this->log_store_->GetFirstEty();

      this->store_->ProductSST(snap_db_path_, "/sst_send/");

      //
      // loop send sst files
      //
      auto snap_files =
          DirectoryTool::ListDirFiles(snap_db_path_ + "/sst_send/");
      for (auto snapfile : snap_files) {
        if (StringUtil::endsWith(snapfile, ".sst")) {
          SPDLOG_INFO("snapfile {}", snapfile);
          this->net_->SendFile(this, node, snapfile);
        }
      }

      eraftkv::SnapshotReq* snap_req = new eraftkv::SnapshotReq();
      snap_req->set_term(this->current_term_);
      snap_req->set_leader_id(this->id_);
      snap_req->set_last_included_index(new_first_log_ent->id());
      snap_req->set_last_included_term(new_first_log_ent->term());

      SPDLOG_INFO("send snapshot to node {} with req {}",
                  node->id,
                  snap_req->DebugString());

      this->net_->SendSnapshot(this, node, snap_req);

      delete snap_req;

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
  if (!this->IsSnapshoting()) {
    if (this->last_applied_idx_ >= this->commit_idx_) {
      return EStatus::kOk;
    }
    SPDLOG_INFO("appling entries from {} to {}",
                this->last_applied_idx_,
                this->commit_idx_);
    auto etys =
        this->log_store_->Gets(this->last_applied_idx_, this->commit_idx_);
    for (auto ety : etys) {
      switch (ety->e_type()) {
        case eraftkv::EntryType::Normal: {
          eraftkv::KvOpPair* op_pair = new eraftkv::KvOpPair();
          op_pair->ParseFromString(ety->data());
          switch (op_pair->op_type()) {
            case eraftkv::ClientOpType::Put: {
              if (this->store_->PutKV(op_pair->key(), op_pair->value()) ==
                  EStatus::kOk) {
                this->log_store_->PersisLogMetaState(this->commit_idx_,
                                                     ety->id());
                this->last_applied_idx_ = ety->id();
              }
              break;
            }
            case eraftkv::ClientOpType::Del: {
              if (this->store_->DelKV(op_pair->key()) == EStatus::kOk) {
                this->log_store_->PersisLogMetaState(this->commit_idx_,
                                                     ety->id());
                this->last_applied_idx_ = ety->id();
              }
              break;
            }
            default: {
              this->log_store_->PersisLogMetaState(this->commit_idx_,
                                                   ety->id());
              this->last_applied_idx_ = ety->id();
              break;
            }
          }
          {
            // notify to response
            std::lock_guard<std::mutex> lg(*response_ready_mutex_);
            *is_ok_to_response_ = true;
            if ((*response_ready_singals_)[op_pair->op_sign()] != nullptr) {
              (*response_ready_singals_)[op_pair->op_sign()]->notify_one();
            }
          }
          delete op_pair;
          if (this->log_store_->LogCount() > this->snap_threshold_log_count_) {
            this->SnapshotingStart(ety->id());
          }
          break;
        }
        case eraftkv::EntryType::ConfChange: {
          eraftkv::ClusterConfigChangeReq* conf_change_req =
              new eraftkv::ClusterConfigChangeReq();
          conf_change_req->ParseFromString(ety->data());
          this->log_store_->PersisLogMetaState(this->commit_idx_, ety->id());
          this->last_applied_idx_ = ety->id();
          switch (conf_change_req->change_type()) {
            case eraftkv::ChangeType::ServerJoin: {
              if (conf_change_req->server().id() != this->id_) {
                RaftNode* new_node =
                    new RaftNode(conf_change_req->server().id(),
                                 NodeStateEnum::Running,
                                 0,
                                 ety->id(),
                                 conf_change_req->server().address());
                this->net_->InsertPeerNodeConnection(
                    conf_change_req->server().id(),
                    conf_change_req->server().address());
                bool node_exist = false;
                for (auto node : this->nodes_) {
                  if (node->id == new_node->id) {
                    node_exist = true;
                    // reinit node
                    if (node->node_state == NodeStateEnum::Down) {
                      SPDLOG_DEBUG("reinit node {} to running state",
                                   conf_change_req->server().address());
                      node->node_state = NodeStateEnum::Running;
                      node->next_log_index = 0;
                      node->match_log_index = ety->id();
                      node->address = conf_change_req->server().address();
                    }
                  }
                }
                if (!node_exist) {
                  this->nodes_.push_back(new_node);
                }
              }
              break;
            }
            case eraftkv::ChangeType::ServerLeave: {
              auto to_remove_serverid = conf_change_req->server().id();
              for (auto iter = this->nodes_.begin(); iter != this->nodes_.end();
                   iter++) {
                if ((*iter)->id == to_remove_serverid &&
                    conf_change_req->server().id() != this->id_) {
                  (*iter)->node_state = NodeStateEnum::Down;
                }
              }
              break;
            }
            case eraftkv::ChangeType::ShardJoin: {
              std::string key;
              key.append(SG_META_PREFIX);
              key.append(std::to_string(conf_change_req->shard_id()));
              auto        sg = conf_change_req->shard_group();
              std::string val = sg.SerializeAsString();
              this->store_->PutKV(key, val);
              break;
            }
            case eraftkv::ChangeType::ShardLeave: {
              std::string key;
              key.append(SG_META_PREFIX);
              key.append(std::to_string(conf_change_req->shard_id()));
              this->store_->DelKV(key);
              break;
            }
            case eraftkv::ChangeType::SlotMove: {
              auto        sg = conf_change_req->shard_group();
              std::string key = SG_META_PREFIX;
              key.append(std::to_string(conf_change_req->shard_id()));
              auto value = this->store_->GetKV(key);
              if (!value.first.empty()) {
                eraftkv::ShardGroup* old_sg = new eraftkv::ShardGroup();
                old_sg->ParseFromString(value.first);
                // move slot to new sg
                if (sg.id() == old_sg->id()) {
                  for (auto new_slot : sg.slots()) {
                    // check if slot already exists
                    bool slot_already_exists = false;
                    for (auto old_slot : old_sg->slots()) {
                      if (old_slot.id() == new_slot.id()) {
                        slot_already_exists = true;
                      }
                    }
                    // add slot to sg
                    if (!slot_already_exists) {
                      auto add_slot = old_sg->add_slots();
                      add_slot->CopyFrom(new_slot);
                    }
                  }
                  // write back to db
                  EStatus st =
                      this->store_->PutKV(key, old_sg->SerializeAsString());
                  assert(st == EStatus::kOk);
                }
              }
              break;
            }
            case eraftkv::ChangeType::ShardsQuery: {
              break;
            }
            default: {
              break;
            }
          }
          delete conf_change_req;
          break;
        }
        default:
          break;
      }
    }
  }
  return EStatus::kOk;
}

/**
 * @brief
 *
 */
void RaftServer::NotifyToApply() {
  std::lock_guard<std::mutex> lock(this->apply_ready_mtx_);
  this->ready_to_apply_ = true;
  this->apply_ready_cv_.notify_one();
}


/**
 * @brief
 *
 * @param last_idx
 * @param term
 * @return true
 * @return false
 */
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
  resp->set_term(current_term_);
  resp->set_prevote(req->prevote());
  SPDLOG_INFO("handle vote req " + req->DebugString());

  if (this->current_term_ > req->term()) {
    resp->set_vote_granted(false);
    return EStatus::kOk;
  }

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
  if (!req->prevote()) {
    SPDLOG_INFO("peer {} vote {}", this->id_, req->candidtate_id());

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

    auto prev_log_index = node->next_log_index - 1;

    SPDLOG_INFO("node prev_log_index {} node id {}", prev_log_index, node->id);
    SPDLOG_INFO("current node first log index {}",
                this->log_store_->FirstIndex());

    eraftkv::AppendEntriesReq* append_req = new eraftkv::AppendEntriesReq();
    append_req->set_is_heartbeat(true);
    append_req->set_leader_id(this->id_);
    append_req->set_term(this->current_term_);
    append_req->set_leader_commit(this->commit_idx_);
    append_req->set_prev_log_index(prev_log_index);

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
  SPDLOG_INFO("send request vote revice resp {}, from node {}",
              resp->DebugString(),
              from_node->address);

  if (this->role_ == NodeRaftRoleEnum::PreCandidate &&
      req->term() == this->current_term_ + 1 && resp->prevote()) {
    if (resp->vote_granted()) {
      this->granted_votes_ += 1;
      if (this->granted_votes_ > (this->nodes_.size() / 2)) {
        SPDLOG_INFO(" node {} get majority prevotes in term {}",
                    this->id_,
                    this->current_term_);
        this->election_tick_count_ = this->election_timeout_;
      }
    }
  }
  if (this->current_term_ == req->term() &&
      this->role_ == NodeRaftRoleEnum::Candidate && !resp->prevote()) {
    if (resp->vote_granted()) {
      this->granted_votes_ += 1;
      if (this->granted_votes_ > (this->nodes_.size() / 2)) {
        SPDLOG_INFO("node {} get majority votes in term {}",
                    this->id_,
                    this->current_term_);
        this->BecomeLeader();
        this->granted_votes_ = 0;
      }
    } else {
      if (resp->leader_id() != -1) {
        this->leader_id_ = resp->leader_id();
        this->current_term_ = resp->term();
        this->BecomeFollower();
        this->voted_for_ = -1;
        this->store_->SaveRaftMeta(this, this->current_term_, this->voted_for_);
      }
      if (resp->term() >= this->current_term_) {
        this->BecomeFollower();
        this->voted_for_ = -1;
        this->store_->SaveRaftMeta(this, this->current_term_, this->voted_for_);
      }
    }
  }
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
  SPDLOG_INFO("handle ae {}", req->DebugString());
  ResetRandomElectionTimeout();
  election_tick_count_ = 0;

  if (req->is_heartbeat()) {
    SPDLOG_INFO("recv heart beat");
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
    resp->set_term(0);
    resp->set_success(false);
    return EStatus::kOk;
  }
  // after snapshoting GetLastEty()->term() is 0
  if (!(this->MatchLog(req->prev_log_term(), req->prev_log_index()) ||
        this->log_store_->GetLastEty()->term() == 0)) {
    resp->set_success(false);
    if (this->log_store_->LastIndex() < req->prev_log_index()) {
      SPDLOG_INFO("log conflict with index {} term {}",
                  this->log_store_->LastIndex() + 1,
                  -1);
      resp->set_conflict_index(this->log_store_->LastIndex() + 1);
      resp->set_conflict_term(-1);
    } else {
      // set term
      resp->set_conflict_term(
          this->log_store_->Get(req->prev_log_index())->term());
      // find conflict index
      auto index = req->prev_log_index() - 1;
      while (index >= this->log_store_->FirstIndex() &&
             this->log_store_->Get(index)->term() == resp->conflict_term()) {
        index -= 1;
      }
      resp->set_conflict_index(index);
    }
    return EStatus::kOk;
  }

  for (auto ety : req->entries()) {
    this->log_store_->Append(&ety);
    this->log_store_->PersisLogMetaState(this->commit_idx_,
                                         this->last_applied_idx_);
  }
  this->AdvanceCommitIndexForFollower(req->leader_commit());
  resp->set_success(true);
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
    SPDLOG_INFO("send append entry resp {}", resp->DebugString());
    if (resp->success()) {
      for (auto node : this->nodes_) {
        if (node->node_state == NodeStateEnum::Down) {
          continue;
        }
        if (from_node->id == node->id) {
          node->match_log_index = req->prev_log_index() + req->entries().size();
          SPDLOG_INFO("update node {} match_log_index = {}",
                      from_node->id,
                      node->match_log_index);
          node->next_log_index = node->match_log_index + 1;
          this->AdvanceCommitIndexForLeader();
        }
      }
    } else {
      if (resp->term() > req->term()) {
        this->BecomeFollower();
        this->current_term_ = resp->term();
        this->voted_for_ = -1;
        this->store_->SaveRaftMeta(this, this->current_term_, this->voted_for_);
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
                if (this->log_store_->Get(i)->term() == resp->conflict_term()) {
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
EStatus RaftServer::HandleSnapshotReq(RaftNode*                   from_node,
                                      const eraftkv::SnapshotReq* req,
                                      eraftkv::SnapshotResp*      resp) {
  // SPDLOG_INFO("handle snapshot req {} ", req->DebugString());
  this->is_snapshoting_ = true;
  resp->set_term(this->current_term_);
  resp->set_success(false);

  if (req->term() < this->current_term_) {
    this->is_snapshoting_ = false;
    return EStatus::kOk;
  }

  if (req->term() > this->current_term_) {
    this->current_term_ = req->term();
    this->voted_for_ = -1;
    this->store_->SaveRaftMeta(this, this->current_term_, this->voted_for_);
  }

  this->BecomeFollower();
  ResetRandomElectionTimeout();

  resp->set_success(true);

  auto snap_files = DirectoryTool::ListDirFiles(snap_db_path_ + "/sst_recv/");
  for (auto snapfile : snap_files) {
    this->store_->IngestSST(snapfile);
  }

  if (req->last_included_index() <= this->commit_idx_) {
    this->is_snapshoting_ = false;
    return EStatus::kOk;
  }

  if (req->last_included_index() > this->log_store_->LastIndex()) {
    this->log_store_->Reinit();
  } else {
    this->log_store_->EraseBefore(req->last_included_index());
  }

  // install snapshot
  this->log_store_->ResetFirstLogEntry(req->last_included_term(),
                                       req->last_included_index());

  this->last_applied_idx_ = req->last_included_index();
  this->commit_idx_ = req->last_included_index();
  this->is_snapshoting_ = false;
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

/**
 * @brief
 *
 * @return EStatus
 */
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
  if (new_commit_index > this->commit_idx_ &&
      this->MatchLog(this->current_term_, new_commit_index)) {
    this->commit_idx_ = new_commit_index;
    this->log_store_->PersisLogMetaState(this->commit_idx_,
                                         this->last_applied_idx_);
    this->NotifyToApply();
  }
  return EStatus::kOk;
}

/**
 * @brief
 *
 * @param leader_commit
 * @return EStatus
 */
EStatus RaftServer::AdvanceCommitIndexForFollower(int64_t leader_commit) {
  int64_t new_commit_index =
      std::min(leader_commit, this->log_store_->GetLastEty()->id());
  if (new_commit_index > this->commit_idx_) {
    this->commit_idx_ = new_commit_index;
    this->log_store_->PersisLogMetaState(this->commit_idx_,
                                         this->last_applied_idx_);
    this->NotifyToApply();
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
                                       eraftkv::SnapshotReq*  req,
                                       eraftkv::SnapshotResp* resp) {
  if (resp != nullptr) {
    SPDLOG_INFO("handle snapshot resp {}", resp->DebugString());
    if (this->role_ == NodeRaftRoleEnum::Leader &&
        this->current_term_ == req->term()) {
      if (resp->term() > this->current_term_) {
        this->BecomeFollower();
        this->current_term_ = resp->term();
        this->voted_for_ = -1;
        this->store_->SaveRaftMeta(this, this->current_term_, this->voted_for_);
      } else {
        for (auto node : this->nodes_) {
          if (from_node->id == node->id) {
            node->match_log_index = req->last_included_index();
            node->next_log_index = req->last_included_index() + 1;
            SPDLOG_INFO("update node {} match_log_index {}, next_log_index{} ",
                        node->address,
                        node->match_log_index,
                        node->next_log_index);
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
  this->leader_id_ = this->id_;
  for (auto node : this->nodes_) {
    node->next_log_index = this->log_store_->LastIndex() + 1;
    node->match_log_index = 0;
  }
  this->SendHeartBeat();
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
  if (this->role_ == NodeRaftRoleEnum::Leader ||
      this->role_ == NodeRaftRoleEnum::Candidate) {
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
  SPDLOG_INFO(
      "{} start a new election in term {}", this->id_, this->current_term_);
  this->granted_votes_ = 1;
  this->leader_id_ = -1;
  this->voted_for_ = this->id_;
  eraftkv::RequestVoteReq* vote_req = new eraftkv::RequestVoteReq();
  if (is_prevote) {
    vote_req->set_term(this->current_term_ + 1);
    vote_req->set_prevote(true);
  } else {
    this->current_term_ += 1;
    vote_req->set_term(this->current_term_);
  }
  vote_req->set_candidtate_id(this->id_);
  vote_req->set_last_log_idx(this->log_store_->LastIndex());
  vote_req->set_last_log_term(this->log_store_->GetLastEty()->term());
  this->store_->SaveRaftMeta(this, this->current_term_, this->voted_for_);

  for (auto node : this->nodes_) {
    if (node->id == this->id_ || this->role_ == NodeRaftRoleEnum::Leader ||
        node->node_state == NodeStateEnum::Down) {
      continue;
    }

    SPDLOG_INFO("send request vote to {} with param {}",
                node->address,
                vote_req->DebugString());
    this->net_->SendRequestVote(this, node, vote_req);
  }

  return EStatus::kOk;
}

/**
 * @brief
 *
 * @param ety_idx
 * @param snapdir
 * @return EStatus
 */
EStatus RaftServer::SnapshotingStart(int64_t ety_idx) {
  this->is_snapshoting_ = true;
  auto snap_index = this->log_store_->FirstIndex();

  if (ety_idx <= snap_index) {
    SPDLOG_WARN("ety index is larger than the first log index");
    this->is_snapshoting_ = false;
    return EStatus::kError;
  }

  SPDLOG_INFO("start snapshoting with index {}", ety_idx);

  this->log_store_->EraseBefore(ety_idx);
  // reset first log index
  this->log_store_->ResetFirstLogEntry(this->current_term_, ety_idx);

  this->store_->CreateCheckpoint(snap_db_path_ + "/check");

  this->is_snapshoting_ = false;

  return EStatus::kOk;
}

std::vector<RaftNode*> RaftServer::GetNodes() {
  return nodes_;
}

/**
 * @brief Get the Leader Id object
 *
 * @return int64_t
 */
int64_t RaftServer::GetLeaderId() {
  return leader_id_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool RaftServer::IsLeader() {
  return leader_id_ == id_;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool RaftServer::IsSnapshoting() {
  return is_snapshoting_;
}

int64_t RaftServer::GetCommitIndex() {
  return commit_idx_;
}

int64_t RaftServer::GetAppliedIndex() {
  return last_applied_idx_;
}


std::vector<eraftkv::Entry*> RaftServer::GetPrefixLogs() {
  return log_store_->Gets(this->log_store_->FirstIndex(),
                          this->log_store_->FirstIndex() + 5);
}

std::vector<eraftkv::Entry*> RaftServer::GetSuffixLogs() {
  return log_store_->Gets(this->log_store_->LastIndex() - 5,
                          this->log_store_->LastIndex());
}
