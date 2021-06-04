
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

NetWork(std::map<uint64_t, std::shared_ptr<RaftContext> > peers, std::map<uint64_t, std::shared_ptr<MemoryStorage> > storage, std::map<Connem, float> dropm, std::map<eraftpb::MessageType, bool> ignorem) {
    this->peers = peers;
    this->storage = storage;
    this->dropm = dropm;
    this->ignorem = ignorem;
}

std::map<uint64_t, std::shared_ptr<RaftContext> > peers;

std::map<uint64_t, std::shared_ptr<MemoryStorage> > storage;

std::map<Connem, float> dropm;

std::map<eraftpb::MessageType, bool> ignorem;

std::function<bool(eraftpb::Message)> msgHook;

};



enum class PeerType {
    None,
    Raft,
    BlackHole,
};

std::vector<uint64_t> IdsBySize(uint64_t size) {
    std::vector<uint64_t> ids;
    ids.resize(size);
    for(uint64_t i = 0; i < size; i++) {
        ids[i] = 1 + i;
    }
    return ids;
}

// newNetworkWithConfig is like newNetwork but calls the given func to
// modify the configuration of any state machines it creates.
// TODO:
std::shared_ptr<NetWork> NewNetworkWithConfig(std::shared_ptr<Config> conf, std::vector<std::shared_ptr<RaftContext> > peers, PeerType pt) {
    uint8_t size = peers.size();
    std::vector<uint64_t> peerAddrs = IdsBySize(size);
    std::map<uint64_t, std::shared_ptr<RaftContext> > npeers;
    std::map<uint64_t, std::shared_ptr<MemoryStorage> > nstorage;
    uint8_t i = 0;
    for(auto p : peers) {
        uint8_t id = peerAddrs[i];
        switch (pt)
        {
        case PeerType::None:
        {
            nstorage[id] = std::make_shared<MemoryStorage>();
            // TODO: if conf != nullptr
            Config c(id, peerAddrs, 10, 1, nstorage[id]);
            std::shared_ptr<RaftContext> sm = std::make_shared<RaftContext>(c);
            // https://docs.microsoft.com/en-us/previous-versions/bb982967(v=vs.140)?redirectedfrom=MSDN
            npeers[id] = sm;
            break;
        }
        case PeerType::Raft:

            break;
        case PeerType::BlackHole:
            break;
        default:
            break;
        }
        i++;
    }
    std::map<Connem, float> dropm;
    std::map<eraftpb::MessageType, bool> ignorem;
    return std::make_shared<NetWork>(npeers, nstorage, dropm, ignorem);
}

} // namespace eraft


// TEST(RaftTests, TestProgressLeader2AB) {
//     std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
//     std::vector<uint64_t> peers = {1, 2};
//     eraft::Config c(1, peers, 5, 1, memSt);
//     std::shared_ptr<eraft::RaftContext> r = std::make_shared<eraft::RaftContext>(c);
//     r->BecomeCandidate();
//     r->BecomeLeader();
//     eraftpb::Message propMsg;
//     propMsg.set_from(1);
//     propMsg.set_to(1);
//     propMsg.set_msg_type(eraftpb::MsgPropose);
//     eraftpb::Entry* ent = propMsg.add_entries();
//     ent->set_data(std::string("foo"));
//     for(uint8_t i = 0; i < 5; i++) {
//         std::shared_ptr<eraft::Progress> pr = r->prs_[r->id_];
//         // ASSERT_EQ(pr->match, i + 1);
//         // ASSERT_EQ(pr->next, pr->match + 1);
//         r->Step(propMsg);
//     }
// }


TEST(RaftTests, MemoryStorage) {
    eraftpb::Entry en1, en2, en3;
    en1.set_term(2);
    en1.set_index(1);

    en2.set_term(1);
    en2.set_index(1);

    en3.set_term(2);
    en3.set_index(2);
    
    std::shared_ptr<eraft::StorageInterface> memSt = std::make_shared<eraft::MemoryStorage>();
    std::cout << memSt->Append(std::vector<eraftpb::Entry>{en1}) << std::endl;
    // std::cout << memSt->Append(std::vector<eraftpb::Entry>{en2}) << std::endl;
    // std::cout << memSt->Append(std::vector<eraftpb::Entry>{en3}) << std::endl;

    std::cout << "LastIndex(): " << memSt->LastIndex() << std::endl;
    std::cout << "FirstIndex(): " << memSt->FirstIndex() << std::endl;
}
