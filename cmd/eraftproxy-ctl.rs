use eraft::kv_client;

use std::thread;

#[tokio::main]
async fn main() {
    thread::spawn(|| {
        for _i in 0..100 {
            let _ = kv_client::send_command(String::from("http://[::1]:8088"));
        }
    }).join().expect("Thread panicked");
    std::thread::sleep(std::time::Duration::from_secs(2));
}
