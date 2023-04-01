/**
 * @file eraftkv_server.cc
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-04-01
 *
 * @copyright Copyright (c) 2023
 *
 */
#include <grpcpp/grpcpp.h>
#include "eraftkv_server.h"

/**
 * @brief
 *
 * @param req
 * @param resp
 * @return grpc::Status
 */
grpc::Status ERaftKvServer::RequestVote(RequestVoteReq*  req,
                                        RequestVoteResp* resp) {
  req.vote_granted = true;
  return grpc::Status::OK;
}

/**
 * @brief
 *
 * @param req
 * @param resp
 * @return grpc::Status
 */
grpc::Status ERaftKvServer::AppendEntries(AppendEntriesReq* req,
                                          RequestVoteResp*  resp){
    // 1.call raft_context_->HandleAppendEntriesReq()
};

/**
 * @brief
 *
 * @param req
 * @param resp
 * @return grpc::Status
 */
grpc::Status ERaftKvServer::Snapshot(SnapshotReq* req, SnapshotResp* resp){
    //  raftcore
    // 1.call raft_context_->HandleSnapshotReq();
};

/**
 * @brief
 *
 * @param req
 * @param resp
 * @return grpc::Status
 */
grpc::Status ERaftKvServer::ProcessRWOperation(ClientOperationReq*  req,
                                               ClientOperationResp* resp){
    // 1. req into log entry
    // 2. call raft_context_->ProposeEntry()
    // 3. wait commit
};

/**
 * @brief
 *
 * @return grpc::Status
 */
grpc::Status ERaftKvServer::ClusterConfigChange(ClusterConfigChangeReq,
                                                ClusterConfigChangeResp) {
  return EStatus::NotSupport();
}

/**
 * @brief
 *
 * @param interval
 * @return EStatus
 */
EStatus ERaftKvServer::InitTicker(int interval){
    // 1.set up raft_context_->RunCycle() run interval with periodic_caller_
};
