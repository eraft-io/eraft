#include <Kv/raft_server.h>
#include <Kv/engines.h>

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

}

bool RaftStorage::CheckResponse(raft_cmdpb::RaftCmdResponse* resp, int reqCount)
{

}

bool RaftStorage::Write(kvrpcpb::Context* ctx, std::vector<Modify> batch) 
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
            putReq.set_cf(pt->cf);
            putReq.set_key(pt->key);
            putReq.set_value(pt->value);
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
    rqh.set_region_id(ctx->region_id());
    rqh.set_allocated_peer(ctx->mutable_peer());
    rqh.set_allocated_region_epoch(ctx->mutable_region_epoch());
    rqh.set_term(ctx->term());

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

StorageReader* RaftStorage::Reader(kvrpcpb::Context* ctx)
{
    raft_cmdpb::RaftRequestHeader rqh;
    rqh.set_region_id(ctx->region_id());
    rqh.set_allocated_peer(ctx->mutable_peer());
    rqh.set_allocated_region_epoch(ctx->mutable_region_epoch());
    rqh.set_term(ctx->term());

    raft_cmdpb::RaftCmdRequest request;
    raft_cmdpb::Request req;
    req.set_cmd_type(raft_cmdpb::CmdType::Snap);
    raft_cmdpb::SnapRequest snapReq;
    req.set_allocated_snap(&snapReq);

    Callback* cb;
    
    this->raftRouter_->SendRaftCommand(&request, cb);
}

bool RaftStorage::Raft(raft_serverpb::RaftMessage* msg)
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

    assert(this->node_->Start(this->engs_, trans));    

}

bool RaftStorage::Stop()
{

}

} // namespace kvserver
