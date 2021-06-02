
#include <RaftCore/Raft.h>
#include <RaftCore/MemoryStorage.h>
#include <gtest/gtest.h>

#include <memory>

namespace eraft
{

struct Connem
{

uint64_t from;

uint64_t to;

};


struct NetWork
{

std::map<uint64_t, StateMachine> peers;

std::map<uint64_t, std::shared_ptr<MemoryStorage> > storage;

std::map<Connem, float> dropm;

std::map<eraftpb::MessageType, bool> ignorem;

std::function<bool(eraftpb::Message)> msgHook;

};

std::shared_ptr<Config> NewTestConfig(uint64_t id, std::vector<uint64_t>& peers, uint64_t election, uint64_t heartbeat, std::shared_ptr<StorageInterface> st) {
    return std::make_shared<Config>(id, peers, election, heartbeat, st);
}

// newNetworkWithConfig is like newNetwork but calls the given func to
// modify the configuration of any state machines it creates.


} // namespace eraft


TEST(RaftTests, TestProgressLeader2AB) {
    std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
    std::vector<uint64_t> peers = {1, 2};
    eraft::Config c(1, peers, 5, 1, memSt);
    std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);
    r->BecomeCandidate();
    r->BecomeLeader();
    eraftpb::Message propMsg;
    propMsg.set_from(1);
    propMsg.set_to(1);
    propMsg.set_msg_type(eraftpb::MsgPropose);
    eraftpb::Entry* ent = propMsg.add_entries();
    ent->set_data(std::string("foo"));
    for(uint8_t i = 0; i < 5; i++) {
        std::shared_ptr<eraft::Progress> pr = r->prs_[r->id_];
        ASSERT_EQ(pr->match, i + 1);
        ASSERT_EQ(pr->next, pr->match + 1);
        r->Step(propMsg);
    }
}
