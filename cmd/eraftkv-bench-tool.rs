use eraft::kv_client;
use eraft::eraft_proto;
use std::thread;
extern crate simplelog;
use simplelog::*;
use std::fs::File;
use std::{env};

#[tokio::main]
async fn main() {
    CombinedLogger::init(
        vec![
            TermLogger::new(LevelFilter::Info, Config::default(), TerminalMode::Mixed, ColorChoice::Auto),
            WriteLogger::new(LevelFilter::Info, Config::default(), File::create("eraftkv-bench.log").unwrap()),
        ]
    ).unwrap();
    // let args: Vec<String> = env::args().collect();
    let rt = tokio::runtime::Runtime::new().unwrap();
    thread::spawn(move || {
        rt.block_on(async{
            let _ = kv_client::send_command(String::from("http://[::1]:8088"), eraft_proto::OpType::OpPut, "testa", "vala").await;
            let _ = kv_client::send_command(String::from("http://[::1]:8088"), eraft_proto::OpType::OpPut, "testb", "valb").await;
            let _ = kv_client::send_command(String::from("http://[::1]:8088"), eraft_proto::OpType::OpPut, "testc", "valc").await;
            let _ = kv_client::send_command(String::from("http://[::1]:8088"), eraft_proto::OpType::OpPut, "testcd", "valcd").await;
            let _ = kv_client::send_command(String::from("http://[::1]:8088"), eraft_proto::OpType::OpPut, "testd", "vald").await;
            let resp = kv_client::send_command(String::from("http://[::1]:8088"), eraft_proto::OpType::OpScan, "", "").await;
            simplelog::info!("{:?}", resp);
        });
    }).join().expect("Thread panicked");
}