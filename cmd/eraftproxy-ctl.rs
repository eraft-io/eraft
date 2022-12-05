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
use std::sync::mpsc;

extern crate simplelog;


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
                let mut sql_stmt = String::from_utf8(query_buf.clone()).unwrap();
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
                            let resp = proxy_executor::exec(stmt_, "http://[::1]:8088").await;
                            tx.send(resp.unwrap()).unwrap();
                        });
                }).join().expect("Thread panicked");

                let mut received = rx.recv().unwrap();
                received.push_str("\r\n");
                socket
                .write_all(received.as_bytes())
                .await
                .expect("failed to write data to socket");
            }

        });

    }



    // Ok(())
}
