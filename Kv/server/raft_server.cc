#include <Kv/raft_server.h>
#include <Kv/engines.h>
#include <Kv/utils.h>
#include <Logger/Logger.h>

#include <cassert>

namespace kvserver
{

RaftStorage::RaftStorage(std::shared_ptr<Config> conf) {
    // TODO: snap
    this->engs_ = std::make_shared<Engines>(conf->dbPath_ + "_raft", conf->dbPath_ + "_kv");
    this->conf_ = conf;
}

RaftStorage::~RaftStorage()
{
    delete regionReader_;
}

bool RaftStorage::CheckResponse(raft_cmdpb::RaftCmdResponse* resp, int reqCount)
{

}

bool RaftStorage::Write(const kvrpcpb::Context& ctx, std::vector<Modify> batch) 
{
    std::vector<raft_cmdpb::Request> reqs;

    for(auto b: batch) {
        switch (b.ot_)
        {
        case OpType::Put:
        {
            struct Put* pt = (struct Put*)b.data_;
            raft_cmdpb::Request req;
            req.set_cmd_type(raft_cmdpb::CmdType::Put);
            raft_cmdpb::PutRequest putReq;
            putReq.set_cf(pt->cf_);
            putReq.set_key(pt->key_);
            putReq.set_value(pt->value_);
            req.set_allocated_put(&putReq);
            reqs.push_back(req);
            break;
        }    

        case OpType::Delete:
        {
            struct Delete* dt = (struct Delete*)b.data_;
            raft_cmdpb::Request req;
            req.set_cmd_type(raft_cmdpb::CmdType::Delete);
            raft_cmdpb::DeleteRequest dtReq;
            dtReq.set_cf(dt->cf);
            dtReq.set_key(dt->key);
            req.set_allocated_delete_(&dtReq);
            reqs.push_back(req);
            break;
        }
        default:
            break;
        }
    }

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
    for(auto r: reqs) {
        request.set_allocated_header(&rqh);
        raft_cmdpb::Request* rq = request.add_requests();
        rq = &r;
    }

    Callback* cb;

    // send batch request to router
    this->raftRouter_->SendRaftCommand(&request, cb);
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
    
    this->raftRouter_->SendRaftCommand(&request, cb);
    auto resp = cb->WaitResp();

    RegionReader* regionReader = new RegionReader(this->engs_, resp->responses()[0].snap().region());
    this->regionReader_ = regionReader;
    return regionReader;
}
 
bool RaftStorage::Raft(const raft_serverpb::RaftMessage* msg)
{
    this->raftRouter_->SendRaftMessage(msg);
}

bool RaftStorage::SnapShot(raft_serverpb::RaftSnapshotData* snap)
{

}

bool RaftStorage::Start()
{
    // TODO: schedulerClient

    // raft system init
    this->raftSystem_ = std::make_shared<RaftStore>(this->conf_);
    // router init
    this->raftRouter_ =  this->raftSystem_->raftRouter_;

    // raft client init
    std::shared_ptr<RaftClient> raftClient = std::make_shared<RaftClient>(this->conf_);

    // server transport init
    std::shared_ptr<ServerTransport> trans = std::make_shared<ServerTransport>(raftClient, raftRouter_);

    this->node_ = std::make_shared<Node>(this->raftSystem_, this->conf_);

    if(this->node_->Start(this->engs_, trans))
    {
        Logger::GetInstance()->INFO("raft storage start succeed!");
    } 
    else 
    {
        Logger::GetInstance()->ERRORS("raft storage start error!");
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
    return GetCF(this->engs_->kvDB_, cf, key);
}

rocksdb::Iterator* RegionReader::IterCF(std::string cf)
{
    return NewCFIterator(this->engs_->kvDB_, cf);
}

void RegionReader::Close()
{

}

} // namespace kvserver
