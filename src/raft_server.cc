#include "raft_server.h"

#include <thread>

#include "util.h"

/**
 * @brief Construct a new Raft Server object
 *
 * @param raft_config
 */
RaftServer::RaftServer(RaftConfig raft_config)
    : id_(raft_config.id)
    , role_(NodeRaftRoleEnum::Follower)
    , current_term_(0)
    , voted_for_(-1)
    , commit_idx_(0)
    , last_applied_idx_(0)
    , tick_count_(0)
    , request_timeout_tick_count_(0)
    , election_timeout_tick_count_(0)
    , leader_id_("")
    , tick_interval_(1000) {
  // for(auto n : raft_config.peer_address_map) {
  //   RaftNode* node = new RaftNode(std::string(""), NodeStateEnum::Init,
  //   int64_t(0), int64_t(0)); this->nodes_.push_back(node);
  // }
}

EStatus RaftServer::RunMainLoop(RaftConfig raft_config) {
  RaftServer* svr = new RaftServer(raft_config);
  std::thread th(&RaftServer::RunCycle, svr);
  th.detach();
}

/**
 * @brief Get the Entries To Be Send object
 *
 * @param node
 * @param index
 * @param count
 * @return std::vector<eraftkv::Entry*>
 */
std::vector<eraftkv::Entry*> RaftServer::GetEntriesToBeSend(RaftNode* node,
                                                            int64_t   index,
                                                            int64_t   count) {}

/**
 * @brief
 *
 * @param term
 * @param vote
 * @return EStatus
 */
EStatus RaftServer::SaveMetaData(int64_t term, int64_t vote) {}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::ReadMetaData() {}

/**
 * @brief
 *
 * @param id
 * @param is_self
 * @return RaftNode*
 */
RaftNode* RaftServer::JoinNode(int64_t id, bool is_self) {}

/**
 * @brief
 *
 * @param node
 * @return EStatus
 */
EStatus RaftServer::RemoveNode(RaftNode* node) {}

/**
 * @brief raft core cycle
 *
 * @return EStatus
 */
EStatus RaftServer::RunCycle() {

  while (true) {
    auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);
    tick_count_ += 1;

    TraceLog("tick count -> ", tick_count_);
    // request timeout
    if (tick_count_ % request_timeout_tick_count_ == 0) {
    }

    if (tick_count_ % election_timeout_tick_count_ == 0) {
    }

    std::this_thread::sleep_until(x);
  }
}


/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::ApplyEntries() {}

/**
 * @brief
 *
 * @param from_node
 * @param req
 * @param resp
 * @return EStatus
 */
EStatus RaftServer::HandleRequestVoteReq(RaftNode*                 from_node,
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
EStatus RaftServer::HandleRequestVoteResp(RaftNode*                 from_node,
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
EStatus RaftServer::HandleAppendEntriesReq(RaftNode*                  from_node,
                                           eraftkv::AppendEntriesReq* req,
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
EStatus RaftServer::HandleAppendEntriesResp(RaftNode* from_node,
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
EStatus RaftServer::HandleSnapshotReq(RaftNode*              from_node,
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
EStatus RaftServer::HandleSnapshotResp(RaftNode*              from_node,
                                       eraftkv::SnapshotResp* resp) {
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
EStatus RaftServer::HandleApplyConfigChange(RaftNode*       from_node,
                                            eraftkv::Entry* ety,
                                            int64_t         ety_index) {}

/**
 * @brief
 *
 * @param ety
 * @return EStatus
 */
EStatus RaftServer::ProposeEntry(eraftkv::Entry* ety) {
  // 1.append entry to log
}


/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::BecomeLeader() {
  // 1.call net_->SendAppendEntries()
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::BecomeFollower() {
  // 1.reset election time out
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::BecomeCandidate() {
  // 1. set status canditate

  // 2. incr current node term + 1
  // call net_->SendRequestVote();
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::BecomePreCandidate() {
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
EStatus RaftServer::ElectionStart(bool is_prevote) {
  // 1. set random election timeout

  // 2. if is_prevote = true, BecomePreCandidate else BecomeCandidate
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::BeginSnapshot() {
  // 1. apply all commited log
  // 2. set up snapshot status
}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::EndSnapshot() {
  // 1. call net_->SendSnapshot()
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool RaftServer::SnapshotRunning() {}

/**
 * @brief Get the Last Applied Entry object
 *
 * @return Entry*
 */
eraftkv::Entry* RaftServer::GetLastAppliedEntry() {}

/**
 * @brief Get the First Entry Idx object
 *
 * @return int64_t
 */
int64_t RaftServer::GetFirstEntryIdx() {}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::RestoreSnapshotAfterRestart() {}

/**
 * @brief
 *
 * @param last_included_term
 * @param last_included_index
 * @return EStatus
 */
EStatus RaftServer::BeginLoadSnapshot(int64_t last_included_term,
                                      int64_t last_included_index) {}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::EndLoadSnapshot() {}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::ProposeReadReq() {}

/**
 * @brief Get the Logs Count Can Snapshot object
 *
 * @return int64_t
 */
int64_t RaftServer::GetLogsCountCanSnapshot() {}

/**
 * @brief
 *
 * @return EStatus
 */
EStatus RaftServer::RestoreLog() {}