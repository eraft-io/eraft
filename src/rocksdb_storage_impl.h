// /**
//  * @file rocksdb_storage_impl.h
//  * @author ERaftGroup
//  * @brief
//  * @version 0.1
//  * @date 2023-03-30
//  *
//  * @copyright Copyright (c) 2023
//  *
//  */

// #ifndef ROCKSDB_STORAGE_IMPL_H_
// #define ROCKSDB_STORAGE_IMPL_H_


// #include "raft_server.h"

// /**
//  * @brief
//  *
//  */
// class RocksDBStorageImpl : public Storage {

//  public:
//   /**
//    * @brief Get the Node Address object
//    *
//    * @param raft
//    * @param id
//    * @return std::string
//    */
//   std::string GetNodeAddress(RaftServer* raft, std::string id);

//   /**
//    * @brief
//    *
//    * @param raft
//    * @param id
//    * @param address
//    * @return EStatus
//    */
//   EStatus SaveNodeAddress(RaftServer* raft,
//                           std::string id,
//                           std::string address);

//   /**
//    * @brief
//    *
//    * @param raft
//    * @param snapshot_index
//    * @param snapshot_term
//    * @return EStatus
//    */
//   EStatus ApplyLog(RaftServer* raft,
//                    int64_t     snapshot_index,
//                    int64_t     snapshot_term);

//   /**
//    * @brief Get the Snapshot Block object
//    *
//    * @param raft
//    * @param node
//    * @param offset
//    * @param block
//    * @return EStatus
//    */
//   EStatus GetSnapshotBlock(RaftServer * raft,
//                            RaftNode*               node,
//                            int64_t                 offset,
//                            eraftkv::SnapshotBlock* block);

//   /**
//    * @brief
//    *
//    * @param raft
//    * @param snapshot_index
//    * @param offset
//    * @param block
//    * @return EStatus
//    */
//   EStatus StoreSnapshotBlock(RaftServer*             raft,
//                              int64_t                 snapshot_index,
//                              int64_t                 offset,
//                              eraftkv::SnapshotBlock* block);

//   /**
//    * @brief
//    *
//    * @param raft
//    * @return EStatus
//    */
//   EStatus ClearSnapshot(RaftServer* raft);

//   /**
//    * @brief
//    *
//    * @return EStatus
//    */
//   EStatus CreateDBSnapshot();

//   /**
//    * @brief
//    *
//    * @param raft
//    * @param term
//    * @param vote
//    * @return EStatus
//    */
//   EStatus SaveRaftMeta(RaftServer* raft, int64_t term, int64_t vote);

//   /**
//    * @brief
//    *
//    * @param raft
//    * @param term
//    * @param vote
//    * @return EStatus
//    */
//   EStatus ReadRaftMeta(RaftServer* raft, int64_t* term, int64_t* vote);


//   EStatus PutKV(std::string key, std::string val);

//   EStatus GetKV(std::string key);

//   /**
//    * @brief Construct a new RocksDB Storage Impl object
//    *
//    * @param db_path
//    */
//   RocksDBStorageImpl(std::string db_path);

//  private:
//   /**
//    * @brief
//    *
//    */
//   std::string db_path_;

//   /**
//    * @brief
//    *
//    */
//   rocksdb::DB* kv_db_;
// };


// /**
//  * @brief
//  *
//  */
// class RocksDBLogStorageImpl : public LogStore {

//  public:
//   /**
//    * @brief
//    *
//    */
//   void Init() {}

//   /**
//    * @brief
//    *
//    */
//   void Free() {}

//   /**
//    * @brief
//    *
//    * @param ety
//    * @return EStatus
//    */
//   EStatus Append(eraftkv::Entry* ety) {}

//   /**
//    * @brief
//    *
//    * @param first_index
//    * @return EStatus
//    */
//   EStatus EraseBefore(int64_t first_index) {}

//   /**
//    * @brief
//    *
//    * @param from_index
//    * @return EStatus
//    */
//   EStatus EraseAfter(int64_t from_index) {}

//   /**
//    * @brief
//    *
//    * @param index
//    * @return eraftkv::Entry*
//    */
//   eraftkv::Entry* Get(int64_t index) {}

//   /**
//    * @brief
//    *
//    * @param start_index
//    * @param end_index
//    * @return std::vector<eraftkv::Entry*>
//    */
//   std::vector<eraftkv::Entry*> Gets(int64_t start_index, int64_t end_index) {}

//   /**
//    * @brief
//    *
//    * @return int64
//    */
//   int64 FirstIndex() {}

//   /**
//    * @brief
//    *
//    * @return int64
//    */
//   int64 LastIndex() {}

//   /**
//    * @brief
//    *
//    * @return int64
//    */
//   int64 LogCount() {}

//  private:
//   /**
//    * @brief
//    *
//    */
//   uint64_t count_;
//   /**
//    * @brief
//    *
//    */
//   std::string node_id_;
//   /**
//    * @brief
//    *
//    */
//   rocksdb::DB* master_log_db_;
//   /**
//    * @brief
//    *
//    */
//   rocksdb::DB* standby_log_db_;
// };

// #endif
