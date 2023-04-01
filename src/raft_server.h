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

#include <cstdint>

#include "estatus.h"
#include "raft_config.h"
#include "raft_log.h"
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
class Network {
 public:
  /**
   * @brief Destroy the Network object
   *
   */
  virtual ~Network() {}

  /**
   * @brief
   *
   * @param raft
   * @param target_node
   * @param req
   * @return EStatus
   */
  virtual EStatus SendRequestVote(RaftServer*              raft,
                                  RaftNode*                target_node,
                                  eraftkv::RequestVoteReq* req) = 0;

  /**
   * @brief
   *
   * @param raft
   * @param target_node
   * @param req
   * @return EStatus
   */
  virtual EStatus SendAppendEntries(RaftServer*                raft,
                                    RaftNode*                  target_node,
                                    eraftkv::AppendEntriesReq* req) = 0;

  /**
   * @brief
   *
   * @param raft
   * @param target_node
   * @param req
   * @return EStatus
   */
  virtual EStatus SendSnapshot(RaftServer*           raft,
                               RaftNode*             target_node,
                               eraftkv::SnapshotReq* req) = 0;
}

/**
 * @brief
 *
 */
class Event {
 public:
  /**
   * @brief Destroy the Event object
   *
   */
  virtual ~Event() {}

  /**
   * @brief
   *
   * @param raft
   * @param state
   */
  virtual void RaftStateChangeEvent(RaftServer* raft, RaftStateEnum* state) = 0;

  /**
   * @brief
   *
   * @param raft
   * @param node
   * @param ety
   */
  virtual void RaftGroupMembershipChangeEvent(RaftServer*     raft,
                                              RaftNode*       node,
                                              eraftkv::Entry* ety) = 0;
}


/**
 * @brief
 *
 */
class Storage {

 public:
  /**
   * @brief Destroy the Storage object
   *
   */
  virtual ~Storage() {}


  /**
   * @brief Get the Node Address object
   *
   * @param raft
   * @param id
   * @return std::string
   */
  virtual std::string GetNodeAddress(RaftServer* raft, std::string id) = 0;

  /**
   * @brief
   *
   * @param raft
   * @param id
   * @param address
   * @return EStatus
   */
  virtual EStatus SaveNodeAddress(RaftServer* raft,
                                  std::string id,
                                  std::string address) = 0;

  /**
   * @brief
   *
   * @param raft
   * @param snapshot_index
   * @param snapshot_term
   * @return EStatus
   */
  virtual EStatus ApplyLog(RaftServer* raft,
                           int64_t     snapshot_index,
                           int64_t     snapshot_term) = 0;

  /**
   * @brief Get the Snapshot Block object
   *
   * @param raft
   * @param node
   * @param offset
   * @param block
   * @return EStatus
   */
  virtual EStatus GetSnapshotBlock(RaftServer*             raft,
                                   RaftNode*               node,
                                   int64_t                 offset,
                                   eraftkv::SnapshotBlock* block) = 0;

  /**
   * @brief
   *
   * @param raft
   * @param snapshot_index
   * @param offset
   * @param block
   * @return EStatus
   */
  virtual EStatus StoreSnapshotBlock(RaftServer*             raft,
                                     int64_t                 snapshot_index,
                                     int64_t                 offset,
                                     eraftkv::SnapshotBlock* block) = 0;

  /**
   * @brief
   *
   * @param raft
   * @return EStatus
   */
  virtual EStatus ClearSnapshot(RaftServer* raft) = 0;

  /**
   * @brief
   *
   * @return EStatus
   */
  virtual EStatus CreateDBSnapshot() = 0;

  /**
   * @brief
   *
   * @param raft
   * @param term
   * @param vote
   * @return EStatus
   */
  virtual EStatus SaveRaftMeta(RaftServer* raft,
                               int64_t     term,
                               int64_t     vote) = 0;

  /**
   * @brief
   *
   * @param raft
   * @param term
   * @param vote
   * @return EStatus
   */
  virtual EStatus ReadRaftMeta(RaftServer* raft,
                               int64_t*    term,
                               int64_t*    vote) = 0;
};

/**
 * @brief
 *
 */
class LogStore {
 public:
  /**
   * @brief Destroy the Log Store object
   *
   */
  virtual ~LogStore() {}

  /**
   * @brief
   *
   */
  virtual void Init() = 0;

  /**
   * @brief
   *
   */
  virtual void Free() = 0;

  /**
   * @brief Append add new entries
   *
   * @param ety
   * @return EStatus
   */
  virtual EStatus Append(eraftkv::Entry* ety) = 0;

  /**
   * @brief EraseBefore erase all entries before the given index
   *
   * @param first_index
   * @return EStatus
   */
  virtual EStatus EraseBefore(int64_t first_index) = 0;

  /**
   * @brief EraseAfter erase all entries after the given index
   *
   * @param from_index
   * @return EStatus
   */
  virtual EStatus EraseAfter(int64_t from_index) = 0;

  /**
   * @brief Get get the given index entry
   *
   * @param index
   * @return eraftkv::Entry*
   */
  virtual eraftkv::Entry* Get(int64_t index) = 0;

  /**
   * @brief Gets get the given index range entry
   *
   * @param start_index
   * @param end_index
   * @return std::vector<eraftkv::Entry*>
   */
  virtual std::vector<eraftkv::Entry*> Gets(int64_t start_index,
                                            int64_t end_index) = 0;

  /**
   * @brief FirstIndex get the first index in the entry
   *
   * @return int64_t
   */
  virtual int64_t FirstIndex() = 0;

  /**
   * @brief LastIndex get the last index in the entry
   *
   * @return int64_t
   */
  virtual int64_t LastIndex() = 0;

  /**
   * @brief LogCount get the number of entries
   *
   * @return int64_t
   */
  virtual int64_t LogCount() = 0;
}

/**
 * @brief
 *
 */
class InternalMemLogStorageImpl : public LogStore {

 private:
  /**
   * @brief the number of entries in memory
   *
   */
  uint64_t count_;

  /**
   * @brief the node id
   *
   */
  std::string node_id_;

  /**
   * @brief master log
   *
   */
  std::vector<eraftkv::Entry> master_log_db_;

  /**
   * @brief standby log when snapshoting
   *
   */
  std::vector<eraftkv::Entry> standby_log_db_;
};

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
  EStatus SaveMetaData(int64 term, int64 vote) {}

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
  RaftNode* JoinNode(int64 id, bool is_self) {}

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
   * @return int64
   */
  int64 GetFirstEntryIdx() {}

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
  EStatus BeginLoadSnapshot(int64 last_included_term,
                            int64 last_included_index) {}

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
   * @return int64
   */
  int64 GetLogsCountCanSnapshot() {}

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
