// Copyright 2015 The etcd Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//
// MIT License

// Copyright (c) 2021 Colin

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

#include <gtest/gtest.h>
#include <raftcore/memory_storage.h>
#include <raftcore/raft.h>
#include <raftcore/util.h>

#include <functional>
#include <memory>

namespace eraft {

struct Connem {
  uint64_t from;

  uint64_t to;
};

struct NetWork {
  NetWork(std::map<uint64_t, std::shared_ptr<RaftContext> > peers,
          std::map<uint64_t, std::shared_ptr<MemoryStorage> > storage,
          std::map<Connem, float> dropm,
          std::map<eraftpb::MessageType, bool> ignorem) {
    this->peers = peers;
    this->storage = storage;
    this->dropm = dropm;
    this->ignorem = ignorem;
  }

  std::map<uint64_t, std::shared_ptr<RaftContext> > peers;

  std::map<uint64_t, std::shared_ptr<MemoryStorage> > storage;

  std::map<Connem, float> dropm;

  std::map<eraftpb::MessageType, bool> ignorem;

};

enum class PeerType {
  None,
  Raft,
  BlackHole,
};

std::vector<uint64_t> IdsBySize(uint64_t size) {
  std::vector<uint64_t> ids;
  ids.resize(size);
  for (uint64_t i = 0; i < size; i++) {
    ids[i] = 1 + i;
  }
  return ids;
}

// newNetworkWithConfig is like newNetwork but calls the given func to
// modify the configuration of any state machines it creates.
// TODO:
std::shared_ptr<NetWork> NewNetworkWithConfig(
    std::shared_ptr<Config> conf,
    std::vector<std::shared_ptr<RaftContext> > peers, PeerType pt) {
  uint8_t size = peers.size();
  std::vector<uint64_t> peerAddrs = IdsBySize(size);
  std::map<uint64_t, std::shared_ptr<RaftContext> > npeers;
  std::map<uint64_t, std::shared_ptr<MemoryStorage> > nstorage;
  uint8_t i = 0;
  for (auto p : peers) {
    uint8_t id = peerAddrs[i];
    switch (pt) {
      case PeerType::None: {
        nstorage[id] = std::make_shared<MemoryStorage>();
        // TODO: if conf != nullptr
        Config c(id, peerAddrs, 10, 1, nstorage[id]);
        std::shared_ptr<RaftContext> sm = std::make_shared<RaftContext>(c);
        // https://docs.microsoft.com/en-us/previous-versions/bb982967(v=vs.140)?redirectedfrom=MSDN
        npeers[id] = sm;
        break;
      }
      case PeerType::Raft:
        break;
      case PeerType::BlackHole:
        break;
      default:
        break;
    }
    i++;
  }
  std::map<Connem, float> dropm;
  std::map<eraftpb::MessageType, bool> ignorem;
  return std::make_shared<NetWork>(npeers, nstorage, dropm, ignorem);
}

}  // namespace eraft

TEST(RaftTests, MemoryStorage) {
  eraftpb::Entry en1, en2, en3;
  en1.set_term(2);
  en1.set_index(1);

  en2.set_term(1);
  en2.set_index(1);

  en3.set_term(2);
  en3.set_index(2);

  std::shared_ptr<eraft::MemoryStorage> memSt =
      std::make_shared<eraft::MemoryStorage>();
  std::cout << memSt->Append(std::vector<eraftpb::Entry>{en1}) << std::endl;
}

// TestLeaderSyncFollowerLog tests that the leader could bring a follower's log
// into consistency with its own.
// Reference: section 5.3, figure 7
TEST(RaftTests, TestLeaderSyncFollowerLog2AB) {
  eraftpb::Entry en_1_1, en_1_2, en_1_3, en_4_4, en_4_5, en_5_6, en_5_7, en_6_8,
      en_6_9, en_6_10;
  // TODO: with mock newwork
}

TEST(RaftTests, TestProtobuf) {
  eraftpb::Message msg;
  msg.set_index(22);
  msg.set_log_term(10);
  std::cout << eraft::MessageToString(msg) << std::endl;
}
