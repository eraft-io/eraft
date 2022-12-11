//  MIT License

//  Copyright (c) 2022 eraft dev group

//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:

//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.

//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.

use eraft::kv_client;
use eraft::eraft_proto;
use std::thread;
extern crate simplelog;
use simplelog::*;
use std::fs::File;

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