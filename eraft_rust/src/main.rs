use eraft_rust::server;
use std::{env, f32::consts::E};

#[macro_use] extern crate log;
extern crate simplelog;

use simplelog::*;

use std::fs::File;

fn main() {
    CombinedLogger::init(
        vec![
            TermLogger::new(LevelFilter::Info, Config::default(), TerminalMode::Mixed, ColorChoice::Auto),
            WriteLogger::new(LevelFilter::Info, Config::default(), File::create("eraft.log").unwrap()),
        ]
    ).unwrap();

    let args: Vec<String> = env::args().collect();

    let server_id: u16 = match args[1].trim().parse() {
        Ok(id) => id,
        Err(err) => {
            simplelog::error!("{:?}", err);
            0
        }
    };

    server::run_server(server_id, args[2].as_str());
}
