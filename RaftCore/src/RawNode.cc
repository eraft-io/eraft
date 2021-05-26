#include <RaftCore/RawNode.h>

namespace eraft
{

    RawNode::RawNode(Config* config) {
        // TODO:
    }

    void RawNode::Tick() {
        // TODO:
    }

    void RawNode::Campaign() {
        // TODO:
    }

    void RawNode::Propose(std::vector<uint8_t> data) {
        // TODO:
    }

    void RawNode::ProposeConfChange(eraftpb::ConfChange cc) {
        // TODO:
    }

    eraftpb::ConfChange* RawNode::ApplyConfChange(eraftpb::ConfChange cc) {
        eraftpb::ConfChange* confChange = nullptr;
        // TODO:
        return confChange;
    }

    void RawNode::Step(eraftpb::Message m) {
        // TODO:
    }

    Ready RawNode::EReady() {
        Ready r;
        // TODO:
        return r;
    }

    bool RawNode::HasReady() {
        // TODO:
        return false;
    }

    void RawNode::Advance(Ready rd) {
        // TODO:
    }

    std::map<uint64_t, Progress> RawNode::GetProgress() {
        std::map<uint64_t, Progress> m;
        // TODO:
        return m;
    }

    void TransferLeader(uint64_t transferee) {
        // TODO:
    }


} // namespace eraft
