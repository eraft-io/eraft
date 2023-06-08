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
 * @file raft_server.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-03-30
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef SRC_RAFT_SERVER_H_
#define SRC_RAFT_SERVER_H_

#include <cstdint>
#include <iostream>

#include "eraftkv.pb.h"
#include "estatus.h"
#include "raft_config.h"
#include "raft_node.h"


enum NodeRaftRoleEnum { None, Follower, PreCandidate, Candidate, Leader };

static std::string NodeRoleToStr(NodeRaftRoleEnum role) {
  switch (role) {
    case NodeRaftRoleEnum::None: {
      return "None";
    }
    case NodeRaftRoleEnum::Follower: {
      return "Follower";
    }
    case NodeRaftRoleEnum::PreCandidate: {
      return "PreCandidate";
    }
    case NodeRaftRoleEnum::Candidate: {
      return "Candidate";
    }
    case NodeRaftRoleEnum::Leader: {
      return "Leader";
    }
    default:
      return "Unknow";
  }
}

/**
 * @brief
 *
 */
class RaftServer {

  friend class RocksDBStorageImpl;

 public:
  /**
   * @brief Construct a new Raft Server object
   *
   * @param raft_config
   */
  RaftServer(RaftConfig raft_config,
             LogStore*  log_store,
             Storage*   store,
             Network*   net);

  /**
   * @brief raft core cycle
   *
   * @return EStatus
   */
  EStatus RunCycle();

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus ApplyEntries();

  /**
   * @brief
   *
   * @param from_node
   * @param req
   * @param resp
   * @return EStatus
   */
  EStatus HandleRequestVoteReq(RaftNode*                      from_node,
                               const eraftkv::RequestVoteReq* req,
                               eraftkv::RequestVoteResp*      resp);
  /**
   * @brief
   *
   * @param from_node
   * @param resp
   * @return EStatus
   */
  EStatus HandleRequestVoteResp(RaftNode*                      from_node,
                                const eraftkv::RequestVoteReq* req,
                                eraftkv::RequestVoteResp*      resp);

  /**
   * @brief
   *
   * @param from_node
   * @param req
   * @param resp
   * @return EStatus
   */
  EStatus HandleAppendEntriesReq(RaftNode*                        from_node,
                                 const eraftkv::AppendEntriesReq* req,
                                 eraftkv::AppendEntriesResp*      resp);
  /**
   * @brief
   *
   * @param from_node
   * @param resp
   * @return EStatus
   */
  EStatus HandleAppendEntriesResp(RaftNode*                   from_node,
                                  eraftkv::AppendEntriesReq*  req,
                                  eraftkv::AppendEntriesResp* resp);

  /**
   * @brief
   *
   * @param from_node
   * @param req
   * @param resp
   * @return EStatus
   */
  EStatus HandleSnapshotReq(RaftNode*              from_node,
                            eraftkv::SnapshotReq*  req,
                            eraftkv::SnapshotResp* resp);


  /**
   * @brief
   *
   * @param from_node
   * @param resp
   * @return EStatus
   */
  EStatus HandleSnapshotResp(RaftNode* from_node, eraftkv::SnapshotResp* resp);

  /**
   * @brief
   *
   * @param from_node
   * @param ety
   * @param ety_index
   * @return EStatus
   */
  EStatus HandleApplyConfigChange(RaftNode*       from_node,
                                  eraftkv::Entry* ety,
                                  int64_t         ety_index);

  /**
   * @brief
   *
   * @param payload
   * @param new_log_index
   * @param new_log_term
   * @param is_success
   * @return EStatus
   */
  EStatus ProposeConfChange(std::string payload,
                            int64_t*    new_log_index,
                            int64_t*    new_log_term,
                            bool*       is_success);


  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus BecomeLeader();

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus BecomeFollower();

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus BecomeCandidate();
  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus BecomePreCandidate();

  /**
   * @brief
   *
   * @param is_prevote
   * @return EStatus
   */
  EStatus ElectionStart(bool is_prevote);

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus BeginSnapshot();
  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus EndSnapshot();
  /**
   * @brief
   *
   * @return true
   * @return false
   */
  bool SnapshotRunning();

  /**
   * @brief Get the Last Applied Entry object
   *
   * @return Entry*
   */
  eraftkv::Entry* GetLastAppliedEntry();
  /**
   * @brief Get the First Entry Idx object
   *
   * @return int64_t
   */
  int64_t GetFirstEntryIdx();

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus RestoreSnapshotAfterRestart();

  /**
   * @brief
   *
   * @param last_included_term
   * @param last_included_index
   * @return EStatus
   */
  EStatus BeginLoadSnapshot(int64_t last_included_term,
                            int64_t last_included_index);

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus EndLoadSnapshot();

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus ProposeReadReq();

  /**
   * @brief Get the Logs Count Can Snapshot object
   *
   * @return int64_t
   */
  int64_t GetLogsCountCanSnapshot();

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus RestoreLog();

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus ResetRandomElectionTimeout();

  /**
   * @brief
   *
   * @param raft_config
   * @return EStatus
   */
  static RaftServer* RunMainLoop(RaftConfig raft_config,
                                 LogStore*  log_store,
                                 Storage*   store,
                                 Network*   net);

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus SendHeartBeat();

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus SendAppendEntries();

  /**
   * @brief
   *
   * @param last_idx
   * @param term
   * @return true
   * @return false
   */
  bool IsUpToDate(int64_t last_idx, int64_t term);

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus AdvanceCommitIndexForLeader();

  /**
   * @brief
   *
   * @param leader_commit
   * @return EStatus
   */
  EStatus AdvanceCommitIndexForFollower(int64_t leader_commit);

  /**
   * @brief
   *
   * @param term
   * @param index
   * @return true
   * @return false
   */
  bool MatchLog(int64_t term, int64_t index);

  /**
   * @brief
   *
   * @param payload
   * @param new_log_index
   * @param new_log_term
   * @param is_success
   * @return EStatus
   */
  EStatus Propose(std::string payload,
                  int64_t*    new_log_index,
                  int64_t*    new_log_term,
                  bool*       is_success);

  /**
   * @brief
   *
   */
  Network* net_;
  /**
   * @brief
   *
   */
  Storage* store_;
  /**
   * @brief
   *
   */
  LogStore* log_store_;

 private:
  /**
   * @brief
   *
   */
  int64_t id_;

  /**
   * @brief
   *
   */
  int64_t current_term_;
  /**
   * @brief
   *
   */
  int64_t voted_for_;

  /**
   * @brief node granted votes count
   *
   */
  int64_t granted_votes_;

  /**
   * @brief
   *
   */
  NodeRaftRoleEnum role_;

  /**
   * @brief
   *
   */
  int64_t commit_idx_;
  /**
   * @brief
   *
   */
  int64_t last_applied_idx_;
  /**
   * @brief
   *
   */
  long last_applied_term_;

  /**
   * @brief
   *
   */
  int64_t heartbeat_tick_count_;

  /**
   * @brief
   *
   */
  int64_t election_tick_count_;

  /**
   * @brief
   *
   */
  int64_t heartbeat_timeout_;

  /**
   * @brief
   *
   */
  int64_t election_timeout_;

  /**
   * @brief
   *
   */
  int64_t base_election_timeout_;

  /**
   * @brief current tick count
   *
   */
  int64_t tick_count_;

  /**
   * @brief tick interval
   *
   */
  int64_t tick_interval_;


  /**
   * @brief
   *
   */
  int64_t leader_id_;
  /**
   * @brief
   *
   */
  int64_t message_index_;
  /**
   * @brief
   *
   */
  int64_t last_acked_message_index_;
  /**
   * @brief
   *
   */
  int64_t node_count_;

  /**
   * @brief
   *
   */
  int64_t max_entries_per_append_req_;
  /**
   * @brief
   *
   */
  std::vector<RaftNode*> nodes_;

  bool election_running_;

  bool open_auto_apply_;

  /**
   * @brief
   *
   */
  RaftConfig config_;
};


#endif  // SRC_RAFT_SERVER_H_
