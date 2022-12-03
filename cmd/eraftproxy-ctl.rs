use std::{env};
use std::thread;

extern crate simplelog;
use simplelog::*;
use std::fs::File;
use sqlparser::dialect::GenericDialect;
use sqlparser::parser::*;
use eraft::proxy_executor;

//
// ./target/debug/eraftproxy-ctl [addr] [sql]
//
#[tokio::main]
async fn main() {
    CombinedLogger::init(
        vec![
            TermLogger::new(LevelFilter::Info, Config::default(), TerminalMode::Mixed, ColorChoice::Auto),
            WriteLogger::new(LevelFilter::Info, Config::default(), File::create("eraftkv-proxy.log").unwrap()),
        ]
    ).unwrap();
    let args: Vec<String> = env::args().collect();

    thread::spawn(move || {
        let dialect = GenericDialect {}; // or AnsiDialect, or your own dialect ...
        let ast = Parser::parse_sql(&dialect, args[2].as_str()).unwrap();
        let stmt_ = &ast[0];
        let rt = tokio::runtime::Runtime::new().unwrap();
        rt.block_on(async{
            let _ = proxy_executor::exec(stmt_, &args[1]).await;
        });
    }).join().expect("Thread panicked");
}
