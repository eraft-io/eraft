use eraft::kv_client;
use eraft::eraft_proto;
use std::thread;

#[tokio::main]
async fn main() {
    let rt = tokio::runtime::Runtime::new().unwrap();
    thread::spawn(move || {
        rt.block_on(async{
            let _ = kv_client::send_command(String::from("http://[::1]:8088"), eraft_proto::OpType::OpPut, "testa", "vala").await;
            let _ = kv_client::send_command(String::from("http://[::1]:8088"), eraft_proto::OpType::OpPut, "testb", "valb").await;
            let _ = kv_client::send_command(String::from("http://[::1]:8088"), eraft_proto::OpType::OpPut, "testc", "valc").await;
            let _ = kv_client::send_command(String::from("http://[::1]:8088"), eraft_proto::OpType::OpPut, "testcd", "valcd").await;
            let _ = kv_client::send_command(String::from("http://[::1]:8088"), eraft_proto::OpType::OpPut, "testd", "vald").await;
            let _ = kv_client::send_command(String::from("http://[::1]:8088"), eraft_proto::OpType::OpScan, "testc", "").await;
        });
    }).join().expect("Thread panicked");
}
