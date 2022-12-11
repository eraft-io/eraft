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

