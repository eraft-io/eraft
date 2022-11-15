use eraft::kv_client;
use std::{env};
use std::thread;

extern crate simplelog;
use simplelog::*;
use std::fs::File;
use sqlparser::dialect::GenericDialect;
use sqlparser::parser::*;
use sqlparser::ast::*;

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

        let sql = "INSERT INTO classtab (
                                        Name, 
                                        Class
                                        ) 
                                        VALUES 
                                        ('Tom', 
                                        'B');";

        let dialect = GenericDialect {}; // or AnsiDialect, or your own dialect ...
        let ast = Parser::parse_sql(&dialect, sql).unwrap();
        simplelog::info!("ast {:?} ", ast);
        let stmt_ = &ast[0];
        match stmt_ {
            Statement::Insert { or: _, into: _, columns, overwrite: _, source, partitioned: _, after_columns: _, table: _, on: _, table_name } => {
                let tab : &Ident =  &table_name.0[0];
                let _cols: &std::vec::Vec<sqlparser::ast::Ident> = columns;
                let mut col_vals: Vec<&String> = vec![];
                let s : &std::boxed::Box<sqlparser::ast::Query> = source;
                let bd: &std::boxed::Box<sqlparser::ast::SetExpr> = &s.body;
                match &**bd {
                    SetExpr::Values(v) =>{
                        for row in &v.0 {
                            for rl in row {
                                match &rl {
                                    Expr::Value(v_) => {
                                        match v_ {
                                            Value::SingleQuotedString(ss) => {
                                                col_vals.push(ss);
                                            },
                                            _ => {},
                                        }
                                    },
                                    _ => {},
                                }
                            }
                        }
                    },
                    _ => {} 
                }
                // sample set
                let _ = kv_client::send_command(String::from("http://[::1]:8088"), eraft::eraft_proto::OpType::OpPut, tab.value.as_str(), col_vals[0].as_str());
            },
            _ => {
            }
        }
        
    }).join().expect("Thread panicked");
    std::thread::sleep(std::time::Duration::from_secs(2));
}
