#ifndef SRC_RAFT_LOG_H_
#define SRC_RAFT_LOG_H_

#include <string>
#include <vector>

#include "eraftkv.pb.h"
#include "estatus.h"
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

  /**
   * @brief Get the First Ety object
   * 
   * @return eraftkv::Entry* 
   */
  virtual eraftkv::Entry* GetFirstEty() = 0;

  /**
   * @brief Get the Last Ety object
   * 
   * @return eraftkv::Entry* 
   */
  virtual eraftkv::Entry* GetLastEty() = 0;

  virtual EStatus PersisLogMetaState(int64_t commit_idx, int64_t applied_idx) = 0;

  virtual EStatus ReadMetaState(int64_t* commit_idx, int64_t* applied_idx) = 0;

};

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

#endif  // SRC_RAFT_LOG_H_
