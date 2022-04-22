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

#include <network/raft_peer.h>

namespace network {

RaftPeer::RaftPeer() {}

RaftPeer::RaftPeer(uint64_t storeId, std::shared_ptr<RaftConfig> cfg,
                   std::shared_ptr<DBEngines> dbEngines,
                   std::shared_ptr<metapb::Region> region) {}

void RaftPeer::InsertPeerCache(metapb::Peer *peer) {}

void RaftPeer::RemovePeerCache(uint64_t peerId) {}

std::shared_ptr<metapb::Peer> RaftPeer::GetPeerFromCache(uint64_t peerId) {}

uint64_t RaftPeer::Destory(std::shared_ptr<DBEngines> engines, bool keepData) {}

void RaftPeer::SetRegion(std::shared_ptr<metapb::Region> region) {}

uint64_t RaftPeer::PeerId() {}

uint64_t RaftPeer::LeaderId() {}

bool RaftPeer::IsLeader() {}

void RaftPeer::Send(std::shared_ptr<RaftTransport> trans,
                    std::vector<eraftpb::Message> msgs) {}

uint64_t RaftPeer::Term() {}

bool RaftPeer::SendRaftMessage(eraftpb::Message msg,
                               std::shared_ptr<RaftTransport> trans) {}

}  // namespace network
