
#include <RaftCore/Raft.h>
#include <RaftCore/MemoryStorage.h>
#include <gtest/gtest.h>

#include <memory>

TEST(RaftTests, TestProgressLeader2AB) {
    std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
    eraft::Config c;
    c.id = 1;
    std::vector<uint64_t> prs = {1, 2};
    c.peers = prs;
    c.electionTick = 5;
    c.heartbeatTick = 1;
    c.storage = memSt;
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
