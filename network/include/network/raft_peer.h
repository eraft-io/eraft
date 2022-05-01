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

#ifndef ERAFT_NETWORK_RAFT_PEER_H_
#define ERAFT_NETWORK_RAFT_PEER_H_

#include <eraftio/metapb.pb.h>
#include <network/db_engines.h>
#include <network/raft_config.h>
#include <network/raft_peer_storage.h>
#include <network/raft_server_transport.h>
#include <raftcore/raw_node.h>

#include <cstdint>
#include <map>
#include <memory>
namespace network {

class RaftPeerStorage;

class RaftServerTransport;

class TransportInterface;

class RaftPeer {
 public:
  RaftPeer();

  RaftPeer(uint64_t storeId, std::shared_ptr<RaftConfig> cfg,
           std::shared_ptr<DBEngines> dbEngines,
           std::shared_ptr<metapb::Region> region);

  void InsertPeerCache(metapb::Peer *peer);

  void RemovePeerCache(uint64_t peerId);

  std::shared_ptr<metapb::Peer> GetPeerFromCache(uint64_t peerId);

  uint64_t Destory(std::shared_ptr<DBEngines> engines, bool keepData);

  void SetRegion(std::shared_ptr<metapb::Region> region);

  uint64_t PeerId();

  uint64_t LeaderId();

  bool IsLeader();

  void Send(std::shared_ptr<TransportInterface> trans,
            std::vector<eraftpb::Message> msgs);

  uint64_t Term();

  std::shared_ptr<metapb::Region> Region();

  void SendRaftMessage(eraftpb::Message msg,
                       std::shared_ptr<TransportInterface> trans);

  std::shared_ptr<eraft::RawNode> GetRaftGroup();

  uint64_t GetRegionID();

  //  private:
  std::shared_ptr<eraft::RawNode> raftGroup_;

  std::shared_ptr<RaftPeerStorage> peerStorage_;

  std::shared_ptr<metapb::Peer> meta_;

  bool stopped_;

  uint64_t regionId_;

  std::map<uint64_t, std::shared_ptr<metapb::Peer> > peerCache_;

  std::string tag_;

  uint64_t lastCompactedIdx_;

  uint64_t approximateSize_;
};

}  // namespace network

#endif
