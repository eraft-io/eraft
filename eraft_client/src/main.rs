use eraft_client::client;
use std::thread;

#[tokio::main]
async fn main() {
    thread::spawn(|| {
        for _i in 0..1000 {
            let _ = client::send_command();
        }
    }).join().expect("Thread panicked");
    std::thread::sleep(std::time::Duration::from_secs(2));
}
