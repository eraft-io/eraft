use sqlparser::ast::*;
use crate::kv_client;
use crate::consts;
use crate::proxy_table;
use crate::{eraft_proto};
extern crate simplelog;
use simplelog::*;

pub struct ExecResult {
    pub res_line: String,
    pub leader_id: i64,
}

pub async fn exec(stmt: &sqlparser::ast::Statement, kv_svr_addrs: &str) -> Result<ExecResult, Box<dyn std::error::Error>> {
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
                SetExpr::Values(v) => {
                    let mut res = ExecResult{
                        res_line: String::from(""),
                        leader_id: 0,
                    };
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
                                            let key = format!("/{}/{}/{}", table_name_ident.value, row_key, cols[count]);
                                            let resp = kv_client::send_command(String::from(kv_svr_addrs), eraft_proto::OpType::OpPut, key.as_str(),ss.as_str()).await?;
                                            simplelog::info!("{:?}", resp);
                                            res.res_line = String::from("ok");
                                            res.leader_id = resp.get_ref().leader_id;
                                        },
                                        _ => {},
                                    }
                                },
                                _ => {},
                            }
                            count += 1;
                        }
                    }
                    return Ok(res)
                },
                _ => {} 
            }
        },

        Statement::Query(s) => {
            let bd: &std::boxed::Box<sqlparser::ast::SetExpr> = &s.body;
            match &**bd {
                SetExpr::Select(sel) => {
                    let mut res = ExecResult{
                        res_line: String::from(""),
                        leader_id: 0,
                    };
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

                    let tb_meta_key = format!("{}{}", consts::TBS_KEY_PREFIX, tb_name);
                    let tb_meta_resp = kv_client::send_command(String::from(kv_svr_addrs), eraft_proto::OpType::OpGet, tb_meta_key.as_str(), "").await?;
                    let proxy_tab: proxy_table::Table = serde_json::from_str(tb_meta_resp.get_ref().value.as_str())?;
                    let key = format!("/{}/{}", tb_name, query_val);
                    let resp = kv_client::send_command(String::from(kv_svr_addrs), eraft_proto::OpType::OpScan, key.as_str(), "").await?;
                    simplelog::info!("{:?}", resp);
                    let mut res_str = String::from("");
                    res_str.push('|');
                    for head in &proxy_tab.column_names {
                        res_str.push_str(head);
                        res_str.push('|');
                    }
                    res_str.push_str(String::from("\r\n").as_str());
                    res_str.push('|');
                    for head in &proxy_tab.column_names {
                        let col_key = key.clone() + "/" + head;
                        let col_resp = kv_client::send_command(String::from(kv_svr_addrs), eraft_proto::OpType::OpGet, col_key.as_str(), "").await?;
                        res_str.push_str(col_resp.get_ref().value.as_str());
                        res_str.push('|');
                   }
                   res.leader_id = resp.get_ref().leader_id;
                   res.res_line = res_str;
                   return Ok(res)
                },
                _ => {}
            }
        },

        Statement::CreateDatabase {
            db_name,
            if_not_exists: _,
            location: _,
            managed_location: _,
        } => {
            let db_name_ident : &Ident =  &db_name.0[0];
            let key = format!("{}{}", consts::DBS_KEY_PREFIX, db_name_ident.value);
            let resp = kv_client::send_command(String::from(kv_svr_addrs), eraft_proto::OpType::OpPut, key.as_str(), "").await?;
            simplelog::info!("{:?}", resp);
        },

        Statement::ShowVariable { variable } => {
            let mut res = ExecResult{
                res_line: String::from(""),
                leader_id: 0,
            };
            if variable.len() == 1 && variable[0].value.eq_ignore_ascii_case("databases") {
                let key = format!("{}", consts::DBS_KEY_PREFIX);
                let resp = kv_client::send_command(String::from(kv_svr_addrs), eraft_proto::OpType::OpScan, key.as_str(), "").await?;
                simplelog::info!("{:?}", resp);
                let mut res_str = String::from("");
                for match_key in &resp.get_ref().match_keys {
                    res_str.push_str(match_key.strip_prefix(consts::DBS_KEY_PREFIX).unwrap_or(match_key));
                    res_str.push_str(String::from("\r\n").as_str());
                }
                res.leader_id = resp.get_ref().leader_id;
                res.res_line = res_str;
                return Ok(res)
            }
        }

        Statement::ShowCreate { obj_type, obj_name } => {
            match &obj_type {
                ShowCreateObject::Table => {
                    let tab_name_ident : &Ident =  &obj_name.0[0];
                    let key = format!("{}{}", consts::TBS_KEY_PREFIX, tab_name_ident.value);
                    let resp = kv_client::send_command(String::from(kv_svr_addrs), eraft_proto::OpType::OpGet, key.as_str(), "").await?;
                    simplelog::info!("{:?}", resp);    
                },
                _ => {}
            }
        }

        Statement::CreateTable { or_replace:_, temporary: _, external:_, global:_, if_not_exists:_, name, columns, 
            constraints:_, hive_distribution:_, hive_formats:_, table_properties:_, with_options:_, file_format:_, location:_, 
            query:_, without_rowid:_, like:_, clone:_, engine:_, 
            default_charset:_,collation:_, on_commit:_, on_cluster:_ } => {
            let mut res = ExecResult{
                res_line: String::from(""),
                leader_id: 0,
            };
            let tab_name_ident : &Ident =  &name.0[0];
            let mut proxy_tab = proxy_table::Table{
                columns_num: columns.len() as u16,

                records_num: 0,
            
                name: tab_name_ident.value.clone(),
                
                column_names: vec![],
            
                field_type: vec![],
            
                auto_inc_counter: 0,
            };

            for col in columns {
                proxy_tab.column_names.push(col.name.value.clone());
                match &col.data_type {
                    DataType::Varchar(_) => {
                        proxy_tab.field_type.push(proxy_table::FieldType::TypeVarchar);
                    },
                    DataType::Int(_) => {
                        proxy_tab.field_type.push(proxy_table::FieldType::TypeInt);
                    }
                    DataType::Float(_) => {
                        proxy_tab.field_type.push(proxy_table::FieldType::TypeFloat);
                    }
                    _ => {
                        simplelog::error!("Unsupport data type");
                    }
                }
            }

            let serialized_tab_meta = serde_json::to_string(&proxy_tab).unwrap();

            let key = format!("{}{}", consts::TBS_KEY_PREFIX, proxy_tab.name);
            let resp = kv_client::send_command(String::from(kv_svr_addrs), eraft_proto::OpType::OpPut, key.as_str(), &serialized_tab_meta).await?;
            simplelog::info!("{:?}", resp);
            res.leader_id = resp.get_ref().leader_id;
            res.res_line = String::from("ok");
            return Ok(res);
        }

        _ => {
        }
    }
    
    Ok(ExecResult { res_line: (String::from("ok")), leader_id: (0) })

}
