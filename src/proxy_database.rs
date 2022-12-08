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

impl DBMS {
    pub fn dbs(&self) -> &[Box<DataBase>] {
        self.dbs.as_ref()
    }
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

impl DataBase {
    pub fn name(&self) -> &str {
        self.name.as_ref()
    }

    pub fn is_open(&self) -> bool {
        self.is_open
    }

    pub fn tables(&self) -> &[Box<Table>] {
        self.tables.as_ref()
    }

    pub fn back_conn_addr(&self) -> &str {
        self.back_conn_addr.as_ref()
    }
}

