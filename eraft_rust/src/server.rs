use crate::raftcore::{RaftStack, Peer, Raft};
use std::sync::{mpsc, Arc};
use std::thread;
use std::time::Duration;
use simplelog::*;

use crate::{eraft_proto};

use eraft_proto::raft_service_client::{RaftServiceClient};


use tonic::{transport::{Server, Channel}, Request, Response, Status};
use eraft_proto::raft_service_server::{RaftService, RaftServiceServer};
use eraft_proto::{RequestVoteRequest, RequestVoteResponse};
use eraft_proto::{AppendEntriesRequest, AppendEntriesResponse};
use eraft_proto::{CommandRequest, CommandResponse};

use std::sync::Mutex;


#[derive(Clone)]
struct RaftServiceImpl{
    f: Arc<Mutex<dyn Raft + Send>>,
}

impl RaftServiceImpl {
    fn new(f: impl Raft + Send + 'static) -> Self {
        Self { f: Arc::new(Mutex::new(f)) }
    }
}

#[tonic::async_trait]
impl RaftService for RaftServiceImpl {

    // request vote rpc
    async fn request_vote(
        &self,
        request: Request<RequestVoteRequest>,
    ) -> Result<Response<RequestVoteResponse>, Status> {
        simplelog::info!("request vote req from {:?} with {:?}", request.remote_addr(), request);
        {
            let mut raft_stack = self.f.lock().unwrap();
            let resp = raft_stack.handle_request_vote(request.into_inner());
            Ok(Response::new(resp))
        }
    }

    // append entries
    async fn append_entries(
        &self,
        request: Request<AppendEntriesRequest>,
    ) -> Result<Response<AppendEntriesResponse>, Status> {
        simplelog::info!("append entries req from {:?} with {:?}", request.remote_addr(), request);
        {
            let mut raft_stack = self.f.lock().unwrap();
            let resp = raft_stack.handle_append_enries(request.into_inner());
            Ok(Response::new(resp))
        }
    }

    // do cmd
    async fn do_command(
                 &self,
                   request: Request<CommandRequest>,
    ) -> Result<Response<CommandResponse>, Status> {
        simplelog::info!("do command req from {:?} with {:?}", request.remote_addr(), request);
        let mut raft_stack = self.f.lock().unwrap();

        let resp = CommandResponse{
            value: String::from("ok"),
            leader_id: raft_stack.get_leader_id() as i64,
            err_code: 0
        };

        let (idx, _, is_leader) = raft_stack.propose(String::from("hello eraft rust"));
        simplelog::info!("send log entry with idx {} to raft", idx);

        Ok(Response::new(resp))
    }

}

#[tokio::main]
pub async fn run_server(sid: u16, svr_addr: &str) -> Result<(), Box<dyn std::error::Error>> {
    let peers : Vec<Peer> = vec![
        Peer{
            id: 0,
            addr: String::from("http://[::1]:8088")
        },
        Peer{
            id: 1,
            addr: String::from("http://[::1]:8089")
        },
        Peer{
            id: 2,
            addr: String::from("http://[::1]:8090")
        }           
    ];
    let (tx, rx) = mpsc::sync_channel(2);

    let raft_stack = RaftStack::new(sid, peers, 1, 5, tx);
    let addr = svr_addr.parse().unwrap();

    let raft_service = RaftServiceImpl::new(raft_stack);
    let raft_service_clone = raft_service.clone();

    thread::spawn(move || {
        loop {
            thread::sleep(Duration::from_millis(1000 as u64));
            let mut raft = raft_service_clone.f.lock().unwrap();
            raft.tick_run();
        }
    });

    simplelog::info!("RaftService server listening on {}", addr);

    Server::builder()
    .add_service(RaftServiceServer::new(raft_service))
    .serve(addr).await?;

    Ok(())
}
