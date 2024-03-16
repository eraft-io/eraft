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
#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <thread>

#include "consts.h"
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
    , heartbeat_timeout_(2)
    , election_timeout_(0)
    , base_election_timeout_(10)
    , heartbeat_tick_count_(0)
    , election_tick_count_(0)
    , max_entries_per_append_req_(100)
    , tick_interval_(100)
    , granted_votes_(0)
    , snap_threshold_log_count_(10000)
    , open_auto_apply_(true)
    , is_snapshoting_(false)
    , snap_db_path_(raft_config.snap_path)
    , election_running_(true) {
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

RaftServer::~RaftServer() {
  delete this->log_store_;
  delete this->net_;
  delete this->store_;
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
  std::thread th1(&RaftServer::RunApply, svr);
  th1.detach();
  return svr;
}


void RaftServer::RunApply() {
  while (true) {
    if (open_auto_apply_) {
      this->ApplyEntries();
    }
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
}

/**
 * @brief raft core cycle
 *
 * @return EStatus
 */
EStatus RaftServer::RunCycle() {
  ResetRandomElectionTimeout();
  while (true) {
    SPDLOG_INFO("heartbeat_tick_count_  " +
                std::to_string(heartbeat_tick_count_));
    SPDLOG_INFO("node role " + NodeRoleToStr(role_));
    SPDLOG_INFO("commit idx {} applied idx {}",
                this->commit_idx_,
                this->last_applied_idx_);
    heartbeat_tick_count_ += 1;
    election_tick_count_ += 1;
    if (heartbeat_tick_count_ == heartbeat_timeout_) {
      if (this->role_ == NodeRaftRoleEnum::Leader) {
        SPDLOG_INFO("heartbeat timeout");
        this->SendHeartBeat();
        heartbeat_tick_count_ = 0;
      }
    }
    if (election_tick_count_ >= election_timeout_ && election_running_) {
      if (this->role_ == NodeRaftRoleEnum::Follower) {
        SPDLOG_INFO("start pre election in term {} ", current_term_);
        this->BecomePreCandidate();
        this->ElectionStart(true);
      } else if (this->role_ == NodeRaftRoleEnum::PreCandidate) {
        if (this->granted_votes_ > (this->nodes_.size() / 2)) {
          this->BecomeCandidate();
          this->ElectionStart(false);
        } else {
          this->BecomeFollower();
        }
      } else if (this->role_ == NodeRaftRoleEnum::Candidate) {
        if (this->granted_votes_ > (this->nodes_.size() / 2)) {
          this->BecomeLeader();
        } else {
          this->BecomeFollower();
        }
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

      RocksDBStorageImpl* snapshot_db = new RocksDBStorageImpl(snap_db_path_);
      auto                kvs =
          snapshot_db->PrefixScan("", 0, SNAPSHOTING_KEY_SCAN_PRE_COOUNT);
      DirectoryTool::MkDir("/eraft/data/sst_send/");
      uint64_t count = 1;
      while (kvs.size() != 0) {
        SPDLOG_INFO("scan find {} keys", kvs.size());
        rocksdb::Options       options;
        rocksdb::SstFileWriter sst_file_writer(rocksdb::EnvOptions(), options);
        sst_file_writer.Open("/eraft/data/sst_send/" + std::to_string(count) +
                             ".sst");
        for (auto kv : kvs) {
          SPDLOG_INFO("key {} -> val {}", kv.first, kv.second);
          sst_file_writer.Put(kv.first, kv.second);
        }
        sst_file_writer.Finish();
        kvs = snapshot_db->PrefixScan("",
                                      count * SNAPSHOTING_KEY_SCAN_PRE_COOUNT,
                                      SNAPSHOTING_KEY_SCAN_PRE_COOUNT);
        count += 1;
      }


      //
      // loop send sst files
      //
      auto snap_files = DirectoryTool::ListDirFiles("/eraft/data/sst_send/");
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
      // snap_req->set_data("snapshotdata");

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
    this->store_->ApplyLog(this, 0, 0);
  }
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
    resp->set_success(true);
    if (this->log_store_->LastIndex() < req->prev_log_index()) {
      SPDLOG_INFO("log conflict with index {} term {}",
                  this->log_store_->LastIndex(),
                  this->log_store_->GetLastEty()->term());
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

  auto snap_files = DirectoryTool::ListDirFiles("/eraft/data/sst_recv/");
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

  this->store_->CreateCheckpoint(snap_db_path_);

  this->is_snapshoting_ = false;

  return EStatus::kOk;
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

bool RaftServer::IsLeader() {
  return leader_id_ == id_;
}

bool RaftServer::IsSnapshoting() {
  return is_snapshoting_;
}