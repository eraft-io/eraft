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

use std::{env};
use std::error::Error;
use std::thread;
use simplelog::*;
use std::fs::File;
use sqlparser::dialect::GenericDialect;
use sqlparser::parser::*;
use eraft::proxy_executor;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio::net::TcpListener;
use std::sync::{mpsc, Mutex};

extern crate simplelog;

use lazy_static::lazy_static;
lazy_static! {
    static ref KV_SVR_ADDRS: Mutex<Vec<String>> = Mutex::new(vec![]);
}

use std::sync::atomic::{AtomicUsize, Ordering};
static GLOBAL_LEADER_ID: AtomicUsize = AtomicUsize::new(0);


#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    CombinedLogger::init(
        vec![
            TermLogger::new(LevelFilter::Info, Config::default(), TerminalMode::Mixed, ColorChoice::Auto),
            WriteLogger::new(LevelFilter::Info, Config::default(), File::create("eraftkv-proxy.log").unwrap()),
        ]
    ).unwrap();

    let addr = env::args().nth(1).unwrap_or_else(|| "127.0.0.1:8080".to_string());
    let listener = TcpListener::bind(&addr).await?;
    simplelog::info!("proxy server success listene on: {}", addr);

    {
        let args: Vec<String> = env::args().collect();
        let mut v = KV_SVR_ADDRS.lock().unwrap();
        let svr_addrs: Vec<&str> = args[2].split(",").collect();
        simplelog::info!("svr_addrs {:?}", svr_addrs);
        for s in svr_addrs {
            v.push(String::from(s))
        }
    }

    loop {

        // Asynchronously wait for an inbound socket.
        let (mut socket, _) = listener.accept().await?;

        tokio::spawn(async move {
            let mut buf = vec![0; 1024];

            // In a loop, read data from the socket and write the data back.
            loop {
                let n = socket
                    .read(&mut buf)
                    .await
                    .expect("failed to read data from socket");

                if n == 0 {
                    return;
                }
                let query_buf = &buf[0..n].to_vec();

                let sql_stmt_pas = String::from_utf8(query_buf.clone());

                match sql_stmt_pas {
                    Ok(mut sql_stmt) => {
                        let (tx, rx) = mpsc::channel();
                        thread::spawn(move || {
                            sql_stmt = sql_stmt.replace("\r", "");
                            sql_stmt = sql_stmt.replace("\n", "");
                            simplelog::info!("{:?} ", sql_stmt);
                            let dialect = GenericDialect {}; // or AnsiDialect, or your own dialect ...
                            let ast = Parser::parse_sql(&dialect, &sql_stmt.as_str()).unwrap();
                            let stmt_ = &ast[0];
                            simplelog::info!("{:?} ", stmt_);
                            let rt = tokio::runtime::Runtime::new().unwrap();
        
                            rt.block_on(async{
                                {
                                    let id = GLOBAL_LEADER_ID.load(Ordering::Relaxed);
                                    let v = KV_SVR_ADDRS.lock().unwrap();
                                    simplelog::info!("send to {:?}", id);
                                    let resp = proxy_executor::exec(stmt_, v[id].to_string().as_str()).await;
                                    let exec_res = resp.unwrap();
                                    GLOBAL_LEADER_ID.store(exec_res.leader_id as usize, Ordering::Relaxed);
                                    tx.send(exec_res.res_line).unwrap(); 
                                }
                            });
                        }).join().expect("Thread panicked");
                        let mut received = rx.recv().unwrap();
                        received.push_str("\r\n");
                        socket
                        .write_all(received.as_bytes())
                        .await
                        .expect("failed to write data to socket");
                    },
                    Err(e) => {
                        simplelog::error!("{:?}", e);
                    }
                }
           
            }

        });

    }
}
