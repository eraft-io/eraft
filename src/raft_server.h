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

#ifndef RAFT_SERVER_H_
#define RAFT_SERVER_H_

#include <cstdint>
#include <iostream>

#include "eraftkv.pb.h"
#include "estatus.h"
#include "raft_config.h"
#include "raft_node.h"

/**
 * @brief
 *
 */
enum RaftStateEnum { Follower, PreCandidate, Candidate, Leader };


/**
 * @brief
 *
 */
class RaftServer {

 public:
  /**
   * @brief Construct a new Raft Server object
   *
   * @param raft_config
   */
  RaftServer(RaftConfig* raft_config) {}

  /**
   * @brief Get the Entries To Be Send object
   *
   * @param node
   * @param index
   * @param count
   * @return std::vector<eraftkv::Entry*>
   */
  std::vector<eraftkv::Entry*> GetEntriesToBeSend(RaftNode* node,
                                                  int64_t   index,
                                                  int64_t   count) {}

  /**
   * @brief
   *
   * @param term
   * @param vote
   * @return EStatus
   */
  EStatus SaveMetaData(int64_t term, int64_t vote) {}

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus ReadMetaData() {}

  /**
   * @brief
   *
   * @param id
   * @param is_self
   * @return RaftNode*
   */
  RaftNode* JoinNode(int64_t id, bool is_self) {}

  /**
   * @brief
   *
   * @param node
   * @return EStatus
   */
  EStatus RemoveNode(RaftNode* node) {}

  /**
   * @brief raft core cycle
   *
   * @return EStatus
   */
  EStatus RunCycle() {
    // 1. only one node, to become the leader
    // call BecomeLeader

    // 2. is leader, send append entries to other node
    // call net_-> SendAppendEntries

    // 3. if not leader, inter election
    // call ElectionStart
  }

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus ApplyEntries() {}

  /**
   * @brief
   *
   * @param from_node
   * @param req
   * @param resp
   * @return EStatus
   */
  EStatus HandleRequestVoteReq(RaftNode*                 from_node,
                               eraftkv::RequestVoteReq*  req,
                               eraftkv::RequestVoteResp* resp) {
    // 1.  deal request vote with raft paper description.
  }

  /**
   * @brief
   *
   * @param from_node
   * @param resp
   * @return EStatus
   */
  EStatus HandleRequestVoteResp(RaftNode*                 from_node,
                                eraftkv::RequestVoteResp* resp) {
    // 1.if resp term > this node, call become follower.

    // 2.get majority vote, become candidate or leader.
  }

  /**
   * @brief
   *
   * @param from_node
   * @param req
   * @param resp
   * @return EStatus
   */
  EStatus HandleAppendEntriesReq(RaftNode*                   from_node,
                                 eraftkv::AppendEntriesReq*  req,
                                 eraftkv::AppendEntriesResp* resp) {
    // 1. deal append entries req with raft paper description.
  }

  /**
   * @brief
   *
   * @param from_node
   * @param resp
   * @return EStatus
   */
  EStatus HandleAppendEntriesResp(RaftNode*                   from_node,
                                  eraftkv::AppendEntriesResp* resp) {
    // 1.deal append entries resp with raft paper description.
  }


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
                            eraftkv::SnapshotResp* resp) {
    // 1. deal snapshot req with raft paper description.
  }


  /**
   * @brief
   *
   * @param from_node
   * @param resp
   * @return EStatus
   */
  EStatus HandleSnapshotResp(RaftNode* from_node, eraftkv::SnapshotResp* resp) {
    // 1. deal snapshot resp with raft paper description.
  }

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
                                  int64_t         ety_index) {}

  /**
   * @brief
   *
   * @param ety
   * @return EStatus
   */
  EStatus ProposeEntry(eraftkv::Entry* ety) {
    // 1.append entry to log
  }


  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus BecomeLeader() {
    // 1.call net_->SendAppendEntries()
  }

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus BecomeFollower() {
    // 1.reset election time out
  }

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus BecomeCandidate() {
    // 1. set status canditate

    // 2. incr current node term + 1
    // call net_->SendRequestVote();
  }

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus BecomePreCandidate() {
    // 1. set status pre canditate

    // 2. set request vote without current node term + 1
    // call net_->SendRequestVote();
  }

  /**
   * @brief
   *
   * @param is_prevote
   * @return EStatus
   */
  EStatus ElectionStart(bool is_prevote) {
    // 1. set random election timeout

    // 2. if is_prevote = true, BecomePreCandidate else BecomeCandidate
  }

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus BeginSnapshot() {
    // 1. apply all commited log
    // 2. set up snapshot status
  }

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus EndSnapshot() {
    // 1. call net_->SendSnapshot()
  }

  /**
   * @brief
   *
   * @return true
   * @return false
   */
  bool SnapshotRunning() {}

  /**
   * @brief Get the Last Applied Entry object
   *
   * @return Entry*
   */
  Entry* GetLastAppliedEntry() {}

  /**
   * @brief Get the First Entry Idx object
   *
   * @return int64_t
   */
  int64_t GetFirstEntryIdx() {}

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus RestoreSnapshotAfterRestart() {}

  /**
   * @brief
   *
   * @param last_included_term
   * @param last_included_index
   * @return EStatus
   */
  EStatus BeginLoadSnapshot(int64_t last_included_term,
                            int64_t last_included_index) {}

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus EndLoadSnapshot() {}

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus ProposeReadReq() {}

  /**
   * @brief Get the Logs Count Can Snapshot object
   *
   * @return int64_t
   */
  int64_t GetLogsCountCanSnapshot() {}

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus RestoreLog() {}

 private:
  /**
   * @brief
   *
   */
  std::string id_;

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
  int64_t last_applied_term_;
  /**
   * @brief
   *
   */
  RaftStateEnum state_;
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
  std::vector<RaftNode> nodes_;
  /**
   * @brief
   *
   */
  RaftConfig config_;
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
};


#endif  // RAFT_SERVER_H_
