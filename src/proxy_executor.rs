use sqlparser::ast::*;
use crate::kv_client;
use crate::consts;
use crate::{eraft_proto};
extern crate simplelog;
use simplelog::*;

pub async fn exec(stmt: &sqlparser::ast::Statement, kv_svr_addrs: &str) -> Result<(), Box<dyn std::error::Error>> {
    match stmt {
        Statement::Insert { or: _, into: _, columns, overwrite: _, source, partitioned: _, after_columns: _, table: _, on: _, table_name } => {
            let table_name_ident : &Ident =  &table_name.0[0];
            let cols: &std::vec::Vec<sqlparser::ast::Ident> = columns;
            let mut row_vals: String = String::new();
            let s : &std::boxed::Box<sqlparser::ast::Query> = source;
            let bd: &std::boxed::Box<sqlparser::ast::SetExpr> = &s.body;
            let mut count = 0;
            let mut row_key : String = String::new();
            match &**bd {
                SetExpr::Values(v) =>{
                    row_vals.push('|');
                    for row in &v.0 {
                        for rl in row {
                            match &rl {
                                Expr::Value(v_) => {
                                    match v_ {
                                        Value::SingleQuotedString(ss) => {
                                            // /table_name/row_key/feild_name    feild_value
                                            if count == 0 {
                                                row_key = String::from(ss.as_str());
                                            }
                                            simplelog::info!("/{}/{}/{} => {}", table_name_ident.value, row_key, cols[count], ss.as_str());
                                        },
                                        _ => {},
                                    }
                                },
                                _ => {},
                            }
                            count += 1;
                        }
                    }
                },
                _ => {} 
            }
            // let key = format!("{}_p_{}", table_name_ident.value, col_vals[0]);
            // sample set
            // let _ = kv_client::send_command(String::from(kv_svr_addrs), eraft_proto::OpType::OpPut, key.as_str(), row_vals.as_str()).await?;
        },

        Statement::Query(s) => {
            let bd: &std::boxed::Box<sqlparser::ast::SetExpr> = &s.body;
            match &**bd {
                SetExpr::Select(sel) => {
                    let mut tb_name = String::new();
                    let mut query_val = String::new();
                    match &sel.from[0].relation {
                        TableFactor::Table { name, alias: _, args: _, with_hints: _ } => {
                            let table_name_ident : &Ident =  &name.0[0];  
                            tb_name = table_name_ident.value.clone();
                        },
                        _ => {}
                    }
                    if let Some(ref selection) = sel.selection {
                        match selection {
                            Expr::BinaryOp { left, op, right } => {
                                match &**left {
                                    _ => {}
                                }
                                match op {
                                    BinaryOperator::Eq => {
                                        // simplelog::info!("op eq");
                                    },
                                    _ => {}
                                }
                                match &**right {
                                    Expr::Value(v_) => {
                                        match v_ {
                                            Value::SingleQuotedString(lv) => {
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
                    let _ = kv_client::send_command(String::from(kv_svr_addrs), eraft_proto::OpType::OpGet, key.as_str(), "").await?;
                },
                _ => {}
            }
        },

        Statement::CreateDatabase {
            db_name,
            if_not_exists,
            location,
            managed_location,
        } => {
            let db_name_ident : &Ident =  &db_name.0[0];
            let key = format!("{}{}", consts::DBS_KEY_PREFIX, db_name_ident.value);
            let _ = kv_client::send_command(String::from(kv_svr_addrs), eraft_proto::OpType::OpPut, key.as_str(), "").await?;
        },

        _ => {
        }
    }
    Ok(())
}
