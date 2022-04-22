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

#ifndef ERAFT_NETWORK_RAFT_SERVER_TRANSPORT_H_
#define ERAFT_NETWORK_RAFT_SERVER_TRANSPORT_H_

#include <eraftio/raft_messagepb.pb.h>
#include <network/raft_router.h>
#include <network/raft_client.h>
#include <memory>

namespace network
{
    class RaftTransport
    {

    public:
        virtual void Send(std::shared_ptr<raft_messagepb::RaftMessage> msg) = 0;
    };

    class RaftServerTransport : public RaftTransport
    {

    public:
        RaftServerTransport(std::shared_ptr<RaftClient> raftClient, std::shared_ptr<RaftRouter> raftRouter);

        ~RaftServerTransport();

        void Send(std::shared_ptr<raft_messagepb::RaftMessage> msg) override;

        void SendStore(uint64_t storeId, std::shared_ptr<raft_messagepb::RaftMessage> msg, std::string addr);

        void WriteData(uint64_t storeId, std::string addr, std::shared_ptr<raft_messagepb::RaftMessage> msg);

    private:
        std::shared_ptr<RaftClient> raftClient_;

        std::shared_ptr<RaftRouter> raftRouter_;
    };

} // namespace network

#endif
