#ifndef ERAFT_KV_PEERSTORAGE_H_
#define ERAFT_KV_PEERSTORAGE_H_

#include <iostream>
#include <eraftio/metapb.grpc.pb.h>
#include <eraftio/raft_serverpb.pb.h>

namespace kvserver
{

class PeerStorage
{
public:
    PeerStorage(/* args */);

    ~PeerStorage();

private:

    std::shared_ptr<metapb::Region> region_;

    std::shared_ptr<raft_serverpb::RaftLocalState> raftState_;

    std::shared_ptr<raft_serverpb::RaftApplyState> applyState_;

    uint64_t snapTriedCnt_;

    std::string tag_;
};

} // namespace kvserver

#endif