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

#ifndef ERAFT_KV_PEER_H_
#define ERAFT_KV_PEER_H_

#include <Kv/callback.h>
#include <Kv/config.h>
#include <Kv/engines.h>
#include <Kv/peer_storage.h>
#include <Kv/transport.h>
#include <RaftCore/raw_node.h>
#include <eraftio/metapb.pb.h>
#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace kvserver {

class PeerStorage;

struct Proposal {
  uint64_t index_;

  uint64_t term_;

  Callback* cb_;
};

class Peer {
  friend class Router;

 public:
  Peer();

  Peer(uint64_t storeID, std::shared_ptr<Config> cfg,
       std::shared_ptr<Engines> engines,
       std::shared_ptr<metapb::Region> region);
  ~Peer();

  void InsertPeerCache(metapb::Peer* peer);

  void RemovePeerCache(uint64_t peerID);

  std::shared_ptr<metapb::Peer> GetPeerFromCache(uint64_t peerID);

  uint64_t NextProposalIndex();

  bool MaybeDestory();

  bool Destory(std::shared_ptr<Engines> engine, bool keepData);

  bool IsInitialized();

  uint64_t storeID();

  std::shared_ptr<metapb::Region> Region();

  void SetRegion(std::shared_ptr<metapb::Region> region);

  uint64_t PeerId();

  uint64_t LeaderId();

  bool IsLeader();

  void Send(std::shared_ptr<Transport> trans,
            std::vector<eraftpb::Message> msgs);

  std::vector<std::shared_ptr<metapb::Peer> > CollectPendingPeers();

  void ClearPeersStartPendingTime();

  bool AnyNewPeerCatchUp(uint64_t peerId);

  bool MaydeCampaign(bool parentIsLeader);

  uint64_t Term();

  // TODO: report status to placement driver
  void HeartbeatScheduler();

  bool SendRaftMessage(eraftpb::Message msg, std::shared_ptr<Transport> trans);

  // instance of the raft moudle
  std::shared_ptr<eraft::RawNode> raftGroup_;

  // peer storage  delete static
  std::shared_ptr<PeerStorage> peerStorage_;

  std::shared_ptr<metapb::Peer> meta_;

  // TODO: peers start pending time
  bool stopped_;

  std::vector<Proposal> proposals_;

  uint64_t regionId_;
  // delete static
  std::map<uint64_t, std::shared_ptr<metapb::Peer> > peerCache_;

 private:
  std::string tag_;

  uint64_t lastCompactedIdx_;

  uint64_t sizeDiffHint_;

  uint64_t approximateSize_;
};

}  // namespace kvserver

#endif