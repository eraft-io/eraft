use crate::eraft_proto::OpType;
use crate::raft_core::{RaftStack, Peer, Raft};
use std::sync::{mpsc, Arc};
use std::thread;
use std::time::Duration;

use prost::bytes::BufMut;
use simplelog::*;

use crate::{eraft_proto};
use std::sync::Mutex;


use tonic::{transport::{Server}, Request, Response, Status};
use eraft_proto::raft_service_server::{RaftService, RaftServiceServer};
use eraft_proto::{RequestVoteRequest, RequestVoteResponse};
use eraft_proto::{AppendEntriesRequest, AppendEntriesResponse};
use eraft_proto::{CommandRequest, CommandResponse};

use rocksdb::DB;

use std::sync::atomic::{AtomicUsize, Ordering};
static GLOBAL_ID_COUNTER: AtomicUsize = AtomicUsize::new(0);


use lazy_static::lazy_static;
lazy_static! {
    static ref REPS: Mutex<Vec<String>> = Mutex::new(vec![]);
}

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
        let mut resp = CommandResponse{
            value: String::from(""),
            leader_id: 0,
            err_code: 0,
            match_keys: vec![String::from("")],
            context: String::from("")
        };

        {
            let mut raft_stack = self.f.lock().unwrap();
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
            if !is_leader {
                simplelog::info!("node is not leader cmd idx {}", idx);
                resp.err_code = 2; // is not leader return
                return Ok(Response::new(resp));
            }
        }

        {
            while GLOBAL_ID_COUNTER.load(Ordering::Relaxed) == 0 {}

            {
                let v = REPS.lock().unwrap();
                if v.len() >= 1 {
                    if request.get_ref().op_type as i16 == eraft_proto::OpType::OpScan as i16 {
                        resp.match_keys = v.to_vec();
                    }
                    resp.value = v[0].to_string();
                }
            }

            {
                GLOBAL_ID_COUNTER.store(0, Ordering::Relaxed)
            }
        }

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

    let raft_stack = RaftStack::new(sid, peers, 1, 5, tx);
    let addr = svr_addr.parse().unwrap();
    let db = DB::open_default(format!("./data_{}", svr_addr)).unwrap();

    let raft_service = RaftServiceImpl::new(raft_stack);
    let raft_service_clone = raft_service.clone();
    let raft_service_for_apply = raft_service.clone();

    // tick
    thread::spawn(move || {
        loop {
            thread::sleep(Duration::from_millis(200 as u64));
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
            if op_type == OpType::OpPut as i16 { // put
                let _ = db.put(key, val);
                {
                    GLOBAL_ID_COUNTER.store(2, Ordering::Relaxed);
                    {
                        let mut v = REPS.lock().unwrap();
                        v.clear();
                        v.push(String::from("ok"));
                    }
                }
            }
            if op_type == OpType::OpScan as i16 {
                let mut db_iter = db.raw_iterator();
                db_iter.seek(seek_key.as_bytes());
                GLOBAL_ID_COUNTER.store(2, Ordering::Relaxed);
                {
                    let mut v = REPS.lock().unwrap();
                    v.clear();
                    while db_iter.valid() {
                        let key_vec = &db_iter.key().unwrap();
                        let key_str = String::from_utf8_lossy(key_vec);
                        if key_str.starts_with(seek_key.as_str()) {
                            v.push(key_str.to_string());
                        }
                        db_iter.next()
                    }
                }
            }
            if op_type == OpType::OpGet as i16 { // get
                match db.get(seek_key) {
                    Ok(Some(value)) => {
                        let resp_val = String::from_utf8(value.to_vec()).unwrap();
                        GLOBAL_ID_COUNTER.store(2, Ordering::Relaxed);
                        {
                            let mut v = REPS.lock().unwrap();
                            v.clear();
                            v.push(String::from(&resp_val));
                        }
                    },
                    Ok(None) => { 
                        GLOBAL_ID_COUNTER.store(2, Ordering::Relaxed);
                        {
                            let mut v = REPS.lock().unwrap();
                            v.clear();
                            v.push(String::from("not found"));
                        }
                     },
                    Err(e) => {              
                        GLOBAL_ID_COUNTER.store(2, Ordering::Relaxed);
                        {
                            let mut v = REPS.lock().unwrap();
                            v.clear();
                            v.push(String::from(format!("error {:?}", e).as_str()));
                        }
                    },
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
