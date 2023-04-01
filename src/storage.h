#ifndef SRC_STORAGE_H_
#define SRC_STORAGE_H_
#include <string>

#include "estatus.h"
#include "raft_server.h"

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

#endif  // SRC_STORAGE_H_
