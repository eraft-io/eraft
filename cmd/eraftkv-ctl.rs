use eraft::kv_server;
use std::{env};

extern crate simplelog;

use simplelog::*;

use std::fs::File;

fn main() {

    let args: Vec<String> = env::args().collect();
    let server_id: u16 = match args[1].trim().parse() {
        Ok(id) => id,
        Err(err) => {
            simplelog::error!("{:?}", err);
            0
        }
    };
    CombinedLogger::init(
        vec![
            TermLogger::new(LevelFilter::Info, Config::default(), TerminalMode::Mixed, ColorChoice::Auto),
            WriteLogger::new(LevelFilter::Info, Config::default(), File::create(format!("eraftkv-node-{}.log", server_id)).unwrap()),
        ]
    ).unwrap();

    let _ = kv_server::run_server(server_id, args[2].as_str());
}
