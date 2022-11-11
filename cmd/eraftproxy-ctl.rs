use eraft::kv_client;
use std::{env};
use std::thread;

extern crate simplelog;
use simplelog::*;
use std::fs::File;

//
// ./target/debug/eraftproxy-ctl [op] [key] [..v]
//
#[tokio::main]
async fn main() {
    CombinedLogger::init(
        vec![
            TermLogger::new(LevelFilter::Info, Config::default(), TerminalMode::Mixed, ColorChoice::Auto),
            WriteLogger::new(LevelFilter::Info, Config::default(), File::create("eraftkv-proxy.log").unwrap()),
        ]
    ).unwrap();

    thread::spawn(|| {
        let args: Vec<String> = env::args().collect();
        let op_t = args[1].as_str();
        if op_t == "get" {
            let op_k = args[2].as_str();
            let _ = kv_client::send_command(String::from("http://[::1]:8088"), eraft::eraft_proto::OpType::OpGet, op_k, "");
        }
        if op_t == "set" {
            let op_k = args[2].as_str();
            let op_v = args[3].as_str();
            let _ = kv_client::send_command(String::from("http://[::1]:8088"), eraft::eraft_proto::OpType::OpPut, op_k, op_v);
        }
    }).join().expect("Thread panicked");
    std::thread::sleep(std::time::Duration::from_secs(2));
}
