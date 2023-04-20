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

 public:
  /**
   * @brief Construct a new Raft Server object
   *
   * @param raft_config
   */
  RaftServer(RaftConfig raft_config);

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
                                                  int64_t   count);

  /**
   * @brief
   *
   * @param term
   * @param vote
   * @return EStatus
   */
  EStatus SaveMetaData(int64_t term, int64_t vote);

  /**
   * @brief
   *
   * @return EStatus
   */
  EStatus ReadMetaData();

  /**
   * @brief
   *
   * @param id
   * @param is_self
   * @return RaftNode*
   */
  RaftNode* JoinNode(int64_t id, bool is_self);

  /**
   * @brief
   *
   * @param node
   * @return EStatus
   */
  EStatus RemoveNode(RaftNode* node);

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
   * @return EStatus
   */
  EStatus SendAppendEntries();

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
                               eraftkv::RequestVoteResp* resp);
  /**
   * @brief
   *
   * @param from_node
   * @param resp
   * @return EStatus
   */
  EStatus HandleRequestVoteResp(RaftNode*                 from_node,
                                eraftkv::RequestVoteResp* resp);

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
                                 eraftkv::AppendEntriesResp* resp);
  /**
   * @brief
   *
   * @param from_node
   * @param resp
   * @return EStatus
   */
  EStatus HandleAppendEntriesResp(RaftNode*                   from_node,
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
   * @param ety
   * @return EStatus
   */
  EStatus ProposeEntry(eraftkv::Entry* ety);


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
  static EStatus RunMainLoop(RaftConfig raft_config);

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
  std::string leader_id_;
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
  std::vector<RaftNode*> nodes_;
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


#endif // SRC_RAFT_SERVER_H_
