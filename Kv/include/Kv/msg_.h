#ifndef ERAFT_KV_MSG_H_
#define ERAFT_KV_MSG_H_

#include <stdint.h>

#include <Kv/callback.h>
#include <eraftio/raft_cmdpb.pb.h>

namespace kvserver
{

enum class MsgType 
{
    MsgTypeNull,
    MsgTypeStart,
    MsgTypeTick,
    MsgTypeRaftMessage,
    MsgTypeRaftCmd,
    MsgTypeSplitRegion,
    MsgTypeRegionApproximateSize,
    MsgTypeGcSnap,
    MsgTypeStoreRaftMessage,
    MsgTypeStoreTick,
    MsgTypeStoreStart,
};

struct Msg
{
    MsgType type_;

    uint64_t regionId_;

    void* data_;

    Msg(MsgType tp, void *data)
    : type_(tp), data_(data)
    {
    }

    Msg(MsgType tp, uint64_t regionId, void *data)
    : type_(tp), regionId_(regionId), data_(data)
    {
    }
};

struct MsgRaftCmd {

    raft_cmdpb::RaftCmdRequest* request_;

    Callback* callback_;

    MsgRaftCmd(raft_cmdpb::RaftCmdRequest* request, Callback* callback) {
        this->request_ = request;
        this->callback_ = callback;
    }

};

static Msg NewMsg(MsgType tp, void* data) 
{
    return Msg(tp, data);
}

static Msg NewPeerMsg(MsgType tp, uint64_t regionId, void* data) 
{
    return Msg(tp, regionId, data);
}

    
} // namespace kvserver


#endif
