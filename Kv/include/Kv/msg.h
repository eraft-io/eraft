// MIT License

// Copyright (c) 2021 eraft dev group

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ERAFT_KV_MSG_H_
#define ERAFT_KV_MSG_H_

#include <Kv/callback.h>
#include <eraftio/raft_cmdpb.pb.h>
#include <stdint.h>

namespace kvserver {

enum class MsgType {
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

struct Msg {
  MsgType type_;

  uint64_t regionId_;

  void* data_;

  Msg(MsgType tp, void* data) : type_(tp), data_(data) {}

  Msg() {}

  Msg(MsgType tp, uint64_t regionId, void* data)
      : type_(tp), regionId_(regionId), data_(data) {}

  std::string MsgToString() {
    switch (type_) {
      case MsgType::MsgTypeNull: {
        return "MsgTypeNull";
        break;
      }
      case MsgType::MsgTypeStart: {
        return "MsgTypeStart";
        break;
      }
      case MsgType::MsgTypeTick: {
        return "MsgTypeTick";
        break;
      }
      case MsgType::MsgTypeRaftMessage: {
        return "MsgTypeRaftMessage";
        break;
      }
      case MsgType::MsgTypeRaftCmd: {
        return "MsgTypeRaftCmd";
        break;
      }
      default:
        break;
    }
    return "unknow";
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

static Msg NewMsg(MsgType tp, void* data) { return Msg(tp, data); }

static Msg NewPeerMsg(MsgType tp, uint64_t regionId, void* data) {
  return Msg(tp, regionId, data);
}

struct MsgSplitRegion {
  metapb::RegionEpoch* region_epoch_;
  std::string split_key_;
};

}  // namespace kvserver

#endif
