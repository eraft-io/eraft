#ifndef ERAFT_RAWNODE_H
#define ERAFT_RAWNODE_H

#include <stdint.h>
#include <eraftio/eraftpb.pb.h>
#include <RaftCore/Raft.h>

namespace eraft
{

class Ready : ESoftState, eraftpb::HardState
{
    public:
    
    Ready();
    // Entries specifies entries to be saved to stable storage BEFORE
    // Messages are sent.
    std::vector<eraftpb::Entry> entries;

    eraftpb::Snapshot snapshot;

    std::vector<eraftpb::Entry> committedEntries;

    std::vector<eraftpb::Message> messages;

};

class RawNode {

public:

    RaftContext* raft;

    RawNode(Config* config);

    void Tick();

    void Campaign();

    void Propose(std::vector<uint8_t> data);

    void ProposeConfChange(eraftpb::ConfChange cc);

    eraftpb::ConfChange* ApplyConfChange(eraftpb::ConfChange cc);

    void Step(eraftpb::Message m);

    Ready EReady();

    bool HasReady();

    void Advance(Ready rd);

    std::map<uint64_t, Progress> GetProgress();

    void TransferLeader(uint64_t transferee);

private:

    ESoftState* prevSoftSt;

    eraftpb::HardState* prevHardSt;
};

} // namespace eraft


#endif