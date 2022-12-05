use sqlparser::ast::*;
use crate::kv_client;
use crate::consts;
use crate::proxy_table;
use crate::{eraft_proto};
extern crate simplelog;
use simplelog::*;

pub async fn exec(stmt: &sqlparser::ast::Statement, kv_svr_addrs: &str) -> Result<(String), Box<dyn std::error::Error>> {
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
                                            let key = format!("/{}/{}/{}", table_name_ident.value, row_key, cols[count]);
                                            let resp = kv_client::send_command(String::from(kv_svr_addrs), eraft_proto::OpType::OpPut, key.as_str(),ss.as_str()).await?;
                                            simplelog::info!("{:?}", resp);
                                        },
                                        _ => {},
                                    }
                                },
                                _ => {},
                            }
                            count += 1;
                        }
                    }
                    return Ok(String::from("ok"))
                },
                _ => {} 
            }
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
                    let key = format!("/{}/{}", tb_name, query_val);
                    let mut resp = kv_client::send_command(String::from(kv_svr_addrs), eraft_proto::OpType::OpScan, key.as_str(), "").await?;
                    simplelog::info!("{:?}", resp);
                    let mut res_str = String::from("");
                    for head in &resp.get_mut().match_keys {
                        res_str.push_str(head);
                        res_str.push('|');
                    }
                    return Ok(res_str)
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
            let resp = kv_client::send_command(String::from(kv_svr_addrs), eraft_proto::OpType::OpPut, key.as_str(), "").await?;
            simplelog::info!("{:?}", resp);
        },

        Statement::ShowVariable { variable } => {
            if variable.len() == 1 && variable[0].value.eq_ignore_ascii_case("databases") {
                let key = format!("{}", consts::DBS_KEY_PREFIX);
                let resp = kv_client::send_command(String::from(kv_svr_addrs), eraft_proto::OpType::OpScan, key.as_str(), "").await?;
                simplelog::info!("{:?}", resp);
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

        Statement::CreateTable { or_replace, temporary, external, global, if_not_exists, name, columns, constraints, hive_distribution, hive_formats, table_properties, with_options, file_format, location, query, without_rowid, like, clone, engine, default_charset, collation, on_commit, on_cluster } => {
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
        }

        _ => {
        }
    }
    
    Ok(String::from("ok"))

}
