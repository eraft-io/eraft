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
    , leader_id_("")
    , heartbeat_timeout_(1)
    , election_timeout_(0)
    , base_election_timeout_(5)
    , heartbeat_tick_count_(0)
    , election_tick_count_(0)
    , tick_interval_(1000) {
  // for(auto n : raft_config.peer_address_map) {
  //   RaftNode* node = new RaftNode(n.first, NodeStateEnum::Init,
  //   int64_t(0), int64_t(0)); this->nodes_.push_back(node);
  // }
}

absl::Status RaftServer::ResetRandomElectionTimeout() {
  // make rand election timeout in (election_timeout, 2 * election_timout)
  auto rand_tick =
      RandomNumber::Between(base_election_timeout_, 2 * base_election_timeout_);
  election_timeout_ = rand_tick;
  return absl::OkStatus();
}

absl::Status RaftServer::RunMainLoop(RaftConfig raft_config) {
  RaftServer* svr = new RaftServer(raft_config);
  std::thread th(&RaftServer::RunCycle, svr);
  th.detach();
  return absl::OkStatus();
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
                                                            int64_t   count) {
  return std::vector<eraftkv::Entry*>{};
}

/**
 * @brief
 *
 * @param term
 * @param vote
 * @return absl::Status
 */
absl::Status RaftServer::SaveMetaData(int64_t term, int64_t vote) {
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @return absl::Status
 */
absl::Status RaftServer::ReadMetaData() {
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @param id
 * @param is_self
 * @return RaftNode*
 */
RaftNode* RaftServer::JoinNode(int64_t id, bool is_self) {
  return nullptr;
}

/**
 * @brief
 *
 * @param node
 * @return absl::Status
 */
absl::Status RaftServer::RemoveNode(RaftNode* node) {
  return absl::OkStatus();
}

/**
 * @brief raft core cycle
 *
 * @return absl::Status
 */
absl::Status RaftServer::RunCycle() {
  ResetRandomElectionTimeout();
  while (true) {
    heartbeat_tick_count_ += 1;
    election_tick_count_ += 1;
    if (heartbeat_tick_count_ == heartbeat_timeout_) {
      TraceLog("DEBUG: ", "heartbeat timeout");
      heartbeat_tick_count_ = 0;
    }
    if (election_tick_count_ == election_timeout_) {
      TraceLog("DEBUG: ", "start election in term", current_term_);

      ResetRandomElectionTimeout();
      election_tick_count_ = 0;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @return absl::Status
 */
absl::Status RaftServer::SendAppendEntries() {

  // only leader can set append entries
  if (this->role_ != NodeRaftRoleEnum::Leader) {
    return absl::Status(absl::StatusCode::kNotSupport, "");
  }

  for (auto& node : this->nodes_) {
    if (node->id == this->id_) {
      return absl::Status(absl::StatusCode::kNotSupport, "");
    }

    auto prev_log_index = node->next_log_index - 1;
    if (prev_log_index < this->log_store_->FirstIndex()) {
      TraceLog("send snapshot to node: ", node->id);
    } else {
    }
  }

  return absl::OkStatus();
}

/**
 * @brief
 *
 * @return absl::Status
 */
absl::Status RaftServer::ApplyEntries() {
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @param from_node
 * @param req
 * @param resp
 * @return absl::Status
 */
absl::Status RaftServer::HandleRequestVoteReq(RaftNode*                 from_node,
                                         eraftkv::RequestVoteReq*  req,
                                         eraftkv::RequestVoteResp* resp) {
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @param from_node
 * @param resp
 * @return absl::Status
 */
absl::Status RaftServer::HandleRequestVoteResp(RaftNode*                 from_node,
                                          eraftkv::RequestVoteResp* resp) {
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @param from_node
 * @param req
 * @param resp
 * @return absl::Status
 */
absl::Status RaftServer::HandleAppendEntriesReq(RaftNode*                  from_node,
                                           eraftkv::AppendEntriesReq* req,
                                           eraftkv::AppendEntriesResp* resp) {
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @param from_node
 * @param resp
 * @return absl::Status
 */
absl::Status RaftServer::HandleAppendEntriesResp(RaftNode* from_node,
                                            eraftkv::AppendEntriesResp* resp) {
  return absl::OkStatus();
}


/**
 * @brief
 *
 * @param from_node
 * @param req
 * @param resp
 * @return absl::Status
 */
absl::Status RaftServer::HandleSnapshotReq(RaftNode*              from_node,
                                      eraftkv::SnapshotReq*  req,
                                      eraftkv::SnapshotResp* resp) {
  return absl::OkStatus();
}


/**
 * @brief
 *
 * @param from_node
 * @param resp
 * @return absl::Status
 */
absl::Status RaftServer::HandleSnapshotResp(RaftNode*              from_node,
                                       eraftkv::SnapshotResp* resp) {
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @param from_node
 * @param ety
 * @param ety_index
 * @return absl::Status
 */
absl::Status RaftServer::HandleApplyConfigChange(RaftNode*       from_node,
                                            eraftkv::Entry* ety,
                                            int64_t         ety_index) {
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @param ety
 * @return absl::Status
 */
absl::Status RaftServer::ProposeEntry(eraftkv::Entry* ety) {
  return absl::OkStatus();
}


/**
 * @brief
 *
 * @return absl::Status
 */
absl::Status RaftServer::BecomeLeader() {
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @return absl::Status
 */
absl::Status RaftServer::BecomeFollower() {
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @return absl::Status
 */
absl::Status RaftServer::BecomeCandidate() {
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @return absl::Status
 */
absl::Status RaftServer::BecomePreCandidate() {
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @param is_prevote
 * @return absl::Status
 */
absl::Status RaftServer::ElectionStart(bool is_prevote) {
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @return absl::Status
 */
absl::Status RaftServer::BeginSnapshot() {
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @return absl::Status
 */
absl::Status RaftServer::EndSnapshot() {
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool RaftServer::SnapshotRunning() {
  return false;
}

/**
 * @brief Get the Last Applied Entry object
 *
 * @return Entry*
 */
eraftkv::Entry* RaftServer::GetLastAppliedEntry() {
  return nullptr;
}

/**
 * @brief Get the First Entry Idx object
 *
 * @return int64_t
 */
int64_t RaftServer::GetFirstEntryIdx() {
  return 0;
}

/**
 * @brief
 *
 * @return absl::Status
 */
absl::Status RaftServer::RestoreSnapshotAfterRestart() {
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @param last_included_term
 * @param last_included_index
 * @return absl::Status
 */
absl::Status RaftServer::BeginLoadSnapshot(int64_t last_included_term,
                                      int64_t last_included_index) {
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @return absl::Status
 */
absl::Status RaftServer::EndLoadSnapshot() {
  return absl::OkStatus();
}

/**
 * @brief
 *
 * @return absl::Status
 */
absl::Status RaftServer::ProposeReadReq() {
  return absl::OkStatus();
}

/**
 * @brief Get the Logs Count Can Snapshot object
 *
 * @return int64_t
 */
int64_t RaftServer::GetLogsCountCanSnapshot() {
  return 0;
}

/**
 * @brief
 *
 * @return absl::Status
 */
absl::Status RaftServer::RestoreLog() {
  return absl::OkStatus();
}