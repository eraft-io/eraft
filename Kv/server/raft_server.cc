#include <cassert>

#include <Kv/raft_server.h>
#include <Kv/engines.h>
#include <Kv/utils.h>
#include <Logger/Logger.h>

namespace kvserver
{

RaftStorage::RaftStorage(std::shared_ptr<Config> conf) {
    this->engs_ = std::make_shared<Engines>(conf->dbPath_ + "_raft", conf->dbPath_ + "_kv");
    this->conf_ = conf;
}

RaftStorage::~RaftStorage()
{

}

bool RaftStorage::CheckResponse(raft_cmdpb::RaftCmdResponse* resp, int reqCount)
{

}

bool RaftStorage::Write(const kvrpcpb::Context& ctx, const kvrpcpb::RawPutRequest* put) 
{
    raft_serverpb::RaftMessage* sendMsg = new raft_serverpb::RaftMessage();
    // send raft message
    sendMsg->set_data(put->key());
    sendMsg->set_region_id(1);
    return this->Raft(sendMsg);
}

StorageReader* RaftStorage::Reader(const kvrpcpb::Context& ctx)
{
    raft_cmdpb::RaftRequestHeader rqh;
    rqh.set_region_id(ctx.region_id());
    auto peer = new metapb::Peer(ctx.peer());
    rqh.set_allocated_peer(peer);
    auto regionEpoch = new metapb::RegionEpoch(ctx.region_epoch());
    rqh.set_allocated_region_epoch(regionEpoch);
    rqh.set_term(ctx.term());

    delete peer;
    delete regionEpoch;

    raft_cmdpb::RaftCmdRequest request;
    raft_cmdpb::Request req;
    req.set_cmd_type(raft_cmdpb::CmdType::Snap);
    raft_cmdpb::SnapRequest snapReq;
    req.set_allocated_snap(&snapReq);

    Callback* cb;
    
    this->raftRouter_->SendRaftCommand(nullptr);
    auto resp = cb->WaitResp();

    RegionReader* regionReader = new RegionReader(this->engs_, resp->responses()[0].snap().region());
    this->regionReader_ = regionReader;
    return regionReader;
}
 
bool RaftStorage::Raft(const raft_serverpb::RaftMessage* msg)
{
    return this->raftRouter_->SendRaftMessage(msg);
}

bool RaftStorage::SnapShot(raft_serverpb::RaftSnapshotData* snap)
{

}

bool RaftStorage::Start()
{

    // raft system init
    this->raftSystem_ = std::make_shared<RaftStore>(this->conf_);
    // router init
    this->raftRouter_ =  this->raftSystem_->raftRouter_;

    // raft client init
    std::shared_ptr<RaftClient> raftClient = std::make_shared<RaftClient>(this->conf_);

    this->node_ = std::make_shared<Node>(this->raftSystem_, this->conf_);

    // server transport init
    std::shared_ptr<ServerTransport> trans = std::make_shared<ServerTransport>(raftClient, raftRouter_);

    if(this->node_->Start(this->engs_, trans))
    {
        Logger::GetInstance()->DEBUG_NEW("raft storage start succeed!", __FILE__, __LINE__, "RaftStorage::Start");
    }
    else
    {
        Logger::GetInstance()->DEBUG_NEW("err: raft storage start error!", __FILE__, __LINE__, "RaftStorage::Start");
    }
}

bool RaftStorage::Stop()
{
    // stop worker
}

RegionReader::RegionReader(std::shared_ptr<Engines> engs, metapb::Region region)
{
    this->engs_ = engs;
    this->region_ = region;
}

RegionReader::~RegionReader()
{

}

std::string RegionReader::GetFromCF(std::string cf, std::string key)
{
    return Assistant::GetInstance()->GetCF(this->engs_->kvDB_, cf, key);
}

rocksdb::Iterator* RegionReader::IterCF(std::string cf)
{
    return Assistant::GetInstance()->NewCFIterator(this->engs_->kvDB_, cf);
}

void RegionReader::Close()
{

}

} // namespace kvserver
