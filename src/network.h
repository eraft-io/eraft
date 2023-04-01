#ifndef SRC_NETWORK_H_
#define SRC_NETWORK_H_

#include "estatus.h"
#include "raft_node.h"
#include "raft_server.h"
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
};

#endif  // SRC_NETWORK_H_
