// MIT License

// Copyright (c) 2022 eraft dev group

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

#ifndef ERAFT_NETWORK_MSG_H_
#define ERAFT_NETWORK_MSG_H_

#include <eraftio/metapb.pb.h>
#include <eraftio/raft_messagepb.pb.h>
#include <network/lock_free_queue.h>

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>

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

  void *data_;

  Msg(MsgType tp, void *data) : type_(tp), data_(data) {}

  Msg() {}

  Msg(MsgType tp, uint64_t regionId, void *data)
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
  raft_messagepb::RaftCmdRequest *request_;

  MsgRaftCmd(raft_messagepb::RaftCmdRequest *request) {
    this->request_ = request;
  }
};

static Msg NewMsg(MsgType tp, void *data) { return Msg(tp, data); }

static Msg NewPeerMsg(MsgType tp, uint64_t regionId, void *data) {
  return Msg(tp, regionId, data);
}

struct MsgSplitRegion {
  metapb::RegionEpoch *region_epoch_;
  std::string split_key_;
};

//
// A concurrency queue impl
//

template <typename T>
class Queue {
 public:
  T Pop() {
    std::unique_lock<std::mutex> mlock(mutex_);
    while (queue_.empty()) {
      cond_.wait(mlock);
    }
    auto val = queue_.front();
    queue_.pop();
    return val;
  }

  void Pop(T &item) {
    std::unique_lock<std::mutex> mlock(mutex_);
    while (queue_.empty()) {
      cond_.wait(mlock);
    }
    item = queue_.front();
    queue_.pop();
  }

  void Push(const T &item) {
    std::unique_lock<std::mutex> mlock(mutex_);
    queue_.push(item);
    mlock.unlock();
    cond_.notify_one();
  }

  uint64_t Size() {}

  Queue() = default;

  // disable copying
  Queue(const Queue &) = delete;
  // disable assignment
  Queue &operator=(const Queue &) = delete;

 private:
  std::queue<T> queue_;

  std::mutex mutex_;

  std::condition_variable cond_;
};

class QueueContext {
 public:
  QueueContext(){};

  ~QueueContext(){};

  static QueueContext *GetInstance() {
    if (instance_ == nullptr) {
      instance_ = new QueueContext();
    }
    return instance_;
  }

  // Queue<Msg>& get_peerSender();
  moodycamel::ConcurrentQueue<Msg> &get_peerSender();
  // Queue<Msg>& get_storeSender();
  moodycamel::ConcurrentQueue<Msg> &get_storeSender();
  // Queue<uint64_t>& get_regionIdCh();
  moodycamel::ConcurrentQueue<uint64_t> &get_regionIdCh();

 protected:
  static QueueContext *instance_;

 private:
  // Queue<Msg> peerSender_;
  moodycamel::ConcurrentQueue<Msg> peerSender_;
  // Queue<Msg> storeSender_;
  moodycamel::ConcurrentQueue<Msg> storeSender_;
  // Queue<uint64_t> regionIdCh_;
  moodycamel::ConcurrentQueue<uint64_t> regionIdCh_;
};

#endif
