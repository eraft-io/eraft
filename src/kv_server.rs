use crate::raft_core::{RaftStack, Peer, Raft};
use std::sync::mpsc::SyncSender;
use std::sync::{mpsc, Arc};
use std::thread;
use std::time::Duration;
use async_std::channel::Receiver;
use prost::bytes::BufMut;
use simplelog::*;

use crate::{eraft_proto};

use tonic::{transport::{Server}, Request, Response, Status};
use eraft_proto::raft_service_server::{RaftService, RaftServiceServer};
use eraft_proto::{RequestVoteRequest, RequestVoteResponse};
use eraft_proto::{AppendEntriesRequest, AppendEntriesResponse};
use eraft_proto::{CommandRequest, CommandResponse};

use std::sync::Mutex;
use rocksdb::DB;

#[derive(Clone)]
struct RaftServiceImpl {
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

        let mut resp = CommandResponse{
            value: String::from("ok"),
            leader_id: raft_stack.get_leader_id() as i64,
            err_code: 0
        };

        //  ------------------------------------------------------------------------
        // |  2 bytes  |  8 bytes    |   n bytes   |    8 bytes      |   n bytes    |
        //  ------------------------------------------------------------------------
        // | op type   |  key length |   key       |    val length   |   val        |
        //  ------------------------------------------------------------------------

        let mut propose_seq: Vec<u8> = Vec::new();

        let op_type: i16 = request.get_ref().op_type as i16;  
        propose_seq.put_i16(op_type);
        propose_seq.put_u64(request.get_ref().key.clone().len() as u64);
        propose_seq.put_slice(request.get_ref().key.clone().into_bytes().as_slice());
        propose_seq.put_u64(request.get_ref().value.clone().len() as u64);
        propose_seq.put_slice(request.get_ref().value.clone().into_bytes().as_slice());
        
        let (idx, _, is_leader) = raft_stack.propose(propose_seq);

        simplelog::info!("is_leader: {}, send log entry with idx {} to raft", is_leader, idx);

        // let data = raft_stack.get_resp_ch().recv().unwrap();
        // resp.value = data;
        
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
    let (tx, rx) = mpsc::sync_channel(100);

    let (cmd_tx, cmd_rx) = mpsc::channel();

    let raft_stack = RaftStack::new(sid, peers, 1, 5, tx, cmd_rx);
    let addr = svr_addr.parse().unwrap();
    let db = DB::open_default(format!("./data_{}", svr_addr)).unwrap();

    let raft_service = RaftServiceImpl::new(raft_stack);
    let raft_service_clone = raft_service.clone();
    let raft_service_for_apply = raft_service.clone();

    // tick
    thread::spawn(move || {
        loop {
            thread::sleep(Duration::from_millis(1000 as u64));
            let mut raft = raft_service_clone.f.lock().unwrap();
            raft.tick_run();
        }
    });

    // applier
    thread::spawn(move || {
        loop {
            thread::sleep(Duration::from_millis(200 as u64));
            let mut raft = raft_service_for_apply.f.lock().unwrap();
            raft.applier();
        }
    });

    thread::spawn(move || {
        loop {
            let received = rx.recv().unwrap();
            println!("Got apply idx: {}", received.command_index);
            println!("Got apply key: {:?}", received.command);
            //  ------------------------------------------------------------------------
            // |  2 bytes  |  8 bytes    |   n bytes   |    8 bytes      |   n bytes    |
            //  ------------------------------------------------------------------------
            // | op type   |  key length |   key       |    val length   |   val        |
            //  ------------------------------------------------------------------------
            let op_type = i16::from_be_bytes(received.command[0..2].try_into().unwrap());
            println!("Get op type {:?}", op_type);
            let key_length = u64::from_be_bytes(received.command[2..10].try_into().unwrap());
            println!("Get key len {:?}", key_length);
            let key = String::from_utf8(received.command.as_slice()[10..10+key_length as usize].to_vec()).unwrap();
            println!("Get key {:?}", key);
            let val_length = u64::from_be_bytes(received.command[10+key_length as usize..(10+key_length+8) as usize].try_into().unwrap());
            println!("Get val len {:?}", val_length);
            let val = String::from_utf8(received.command.as_slice()[(10+key_length+8) as usize..(10+key_length+8+val_length) as usize].to_vec()).unwrap();
            println!("Get value {:?}", val);
            let seek_key = key.clone();
            if op_type == 0 { // put
                let _ = db.put(key, val);
                // cmd_tx.send(String::from("ok"));
            }
            if op_type == 2 { // get
                match db.get(seek_key) {
                    Ok(Some(value)) => {
                        let resp_val = String::from_utf8(value.to_vec()).unwrap();
                        // cmd_tx.send(resp_val);
                    },
                    Ok(None) => { cmd_tx.send(String::from("not found!")).unwrap() },
                    Err(e) => { cmd_tx.send(String::from(format!("error -> {:?}", e))).unwrap() },
                 }
            }
        }
    });

    simplelog::info!("RaftService server listening on {}", addr);

    Server::builder()
    .add_service(RaftServiceServer::new(raft_service))
    .serve(addr).await?;

    Ok(())
}
