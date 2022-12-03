use crate::{proxy_table::Table};

pub trait DBMSOpertaion {

    fn create_db(db_name: &String) -> Box<DataBase>;

    fn switch_db(db_name: &String) -> Box<DataBase>;

    fn drop_db(db_name: &String) -> bool;

    fn show_db_info(db_name: &String) -> String;

    fn close_db(db_name: &String) -> bool;

    fn get_cur_db() -> Box<DataBase>;
}

pub struct DBMS {

    dbs: Vec<Box<DataBase>>,

}

pub trait DBOperation {

    fn create_table(tab_name: &String) -> Box<Table>;

    fn show_table_info(tab_name: &String) -> String;

    fn drop_table(tab_name: &String) -> bool;

}

pub struct DataBase {

    name: String,

    is_open: bool,

    tables: Vec<Box<Table>>,

    back_conn_addr: String,
}

