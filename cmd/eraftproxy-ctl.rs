use eraft::kv_client;
use std::{env};
use std::thread;

extern crate simplelog;
use simplelog::*;
use std::fs::File;
use sqlparser::dialect::GenericDialect;
use sqlparser::parser::*;
use sqlparser::ast::*;
use tonic::{Response};

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
        simplelog::info!("ast {:?} ", ast);
        let stmt_ = &ast[0];
        match stmt_ {
            Statement::Insert { or: _, into: _, columns, overwrite: _, source, partitioned: _, after_columns: _, table: _, on: _, table_name } => {
                let table_name_ident : &Ident =  &table_name.0[0];
                let _cols: &std::vec::Vec<sqlparser::ast::Ident> = columns;
                let mut col_vals: Vec<&String> = vec![];
                let mut row_vals: String = String::new();
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
                                                row_vals.push_str(ss.as_str());
                                                row_vals.push('|');
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
                let key = format!("{}_p_{}", table_name_ident.value, col_vals[0]);

                // sample set
                let _ = kv_client::send_command(String::from(args[1].as_str()), eraft::eraft_proto::OpType::OpPut, key.as_str(), row_vals.as_str());
            },

            Statement::Query(s) => {
                let bd: &std::boxed::Box<sqlparser::ast::SetExpr> = &s.body;
                match &**bd {
                    SetExpr::Select(sel) => {
                        let mut tb_name = String::new();
                        let mut query_val = String::new();
                        match &sel.from[0].relation {
                            TableFactor::Table { name, alias, args, with_hints } => {
                                let table_name_ident : &Ident =  &name.0[0];  
                                // simplelog::info!("tab name {:?} ", table_name_ident.value);
                                tb_name = table_name_ident.value.clone();
                            },
                            _ => {}
                        }
                        if let Some(ref selection) = sel.selection {
                            match selection {
                                Expr::BinaryOp { left, op, right } => {
                                    match &**left {
                                        Expr::Identifier(idt) => {
                                            simplelog::info!("left val {:?} ", idt.value);  
                                        },
                                        _ => {}
                                    }
                                    match op {
                                        BinaryOperator::Eq => {
                                            simplelog::info!("op eq");
                                        },
                                        _ => {}
                                    }
                                    match &**right {
                                        Expr::Value(v_) => {
                                            match v_ {
                                                Value::SingleQuotedString(lv) => {
                                                    // simplelog::info!("right val {:?} ", lv);  
                                                    query_val = lv.to_string();
                                                },
                                                _ => {},
                                            }
                                        },
                                        _ => {}
                                    }
                                },
                                _ => {},
                            }
                        }
                        let key = format!("{}_p_{}", tb_name, query_val);
                        let _ = kv_client::send_command(String::from(args[1].as_str()), eraft::eraft_proto::OpType::OpGet, key.as_str(), "");
                    },
                    _ => {}
                }
            },
            _ => {}
        }
    }).join().expect("Thread panicked");
}
