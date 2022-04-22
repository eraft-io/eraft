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

#ifndef ERAFT_NETWORK_RAFT_ROUTER_H_
#define ERAFT_NETWORK_RAFT_ROUTER_H_

#include <network/concurrency_msg_queue.h>
#include <network/raft_peer.h>
#include <cstdint>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>

namespace network
{

    struct RaftPeerState
    {
        RaftPeerState(std::shared_ptr<RaftPeer> peer)
            : closed_(0), peer_(peer)
        {
        }

        std::atomic<uint32_t> closed_;

        std::shared_ptr<RaftPeer> peer_;
    };

    class Router
    {
    public:
        Router();

        std::shared_ptr<RaftPeerState> Get(uint64_t regionId);

        void Register(std::shared_ptr<RaftPeer> peer);

        void Close(uint64_t regionId);

        bool Send(uint64_t regionId, Msg m);

        void SendStore(Msg m);

        ~Router() {}

    protected:
        std::map<uint64_t, std::shared_ptr<RaftPeerState> > peers_;
    };

    class RaftRouter
    {
    public:
        virtual bool Send(uint64_t regionID, Msg m) = 0;

        virtual bool SendRaftMessage(const raft_messagepb::RaftMessage *msg) = 0;
    };

    class RaftStoreRouter : public RaftRouter
    {

    public:
        RaftStoreRouter(std::shared_ptr<Router> r);

        ~RaftStoreRouter();

        bool Send(uint64_t regionId, Msg m) override;

        bool SendRaftMessage(const raft_messagepb::RaftMessage *msg) override;

    private:
        std::shared_ptr<Router> router_;

        std::mutex mtx_;

        static raft_messagepb::RaftMessage *raft_msg_;
    }

} // namespace network

#endif