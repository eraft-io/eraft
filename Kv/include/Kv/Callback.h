#ifndef ERAFT_KV_CALLBACK_H_
#define ERAFT_KV_CALLBACK_H_

#include <eraftio/raft_cmdpb.pb.h>
#include <atomic>

namespace kvserver
{
    
struct Callback
{
    Callback() {
        this->done_ = false;
        this->resp_ = nullptr;
    }

    raft_cmdpb::RaftCmdResponse* resp_;

    std::atomic<bool> done_;

    void Done(raft_cmdpb::RaftCmdResponse* resp) {
        if(resp != nullptr) {
            this->resp_ = resp;
        }
        this->done_ = true;
    }

    raft_cmdpb::RaftCmdResponse* WaitResp() {
        if(this->done_) {
            return resp_;
        }
        return nullptr;
    }

    raft_cmdpb::RaftCmdResponse* WaitRespWithTimeout() {
        if(this->done_) { // TODO: with time check
        }
    }
};


} // namespace kvserver


#endif