#ifndef SRC_NETWORK_H_
#define SRC_NETWORK_H_

#include "absl/status/status.h"
#include "raft_node.h"
#include "raft_server.h"

class RaftServer;
// enum RaftStateEnum;

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
   * @return absl::Status
   */
  virtual absl::Status SendRequestVote(RaftServer*               raft,
                                  RaftNode*                 target_node,
                                  eraftkv::RequestVoteReq*  req) = 0;

  /**
   * @brief
   *
   * @param raft
   * @param target_node
   * @param req
   * @return absl::Status
   */
  virtual absl::Status SendAppendEntries(RaftServer*                 raft,
                                    RaftNode*                   target_node,
                                    eraftkv::AppendEntriesReq*  req) = 0;

  /**
   * @brief
   *
   * @param raft
   * @param target_node
   * @param req
   * @return absl::Status
   */
  virtual absl::Status SendSnapshot(RaftServer*            raft,
                               RaftNode*              target_node,
                               eraftkv::SnapshotReq*  req) = 0;
};

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
  virtual void RaftStateChangeEvent(RaftServer* raft, int state) = 0;

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
};

#endif  // SRC_NETWORK_H_
