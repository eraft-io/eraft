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

use serde::{Serialize, Deserialize};

#[derive(Debug, Serialize, Deserialize)]
pub enum FieldType {
    
    TypeInt,

    TypeFloat,

    TypeVarchar,

    TypeDate,

}

pub trait TabOperation {

    fn create_idx(column_name: &String) -> bool;

    fn drop_idx(column_name: &String) -> bool;

    fn insert_record(record: Vec<(String, String)>) -> bool;

    fn delete_record(match_condition: (String, String)) -> u32;

    fn select_records(query_condition: (String, String)) -> String;

}


#[derive(Serialize, Deserialize, Debug)]
pub struct Table {
    
    // the number of table columns
    pub columns_num: u16,

    pub records_num: u64,

    pub name: String,
    
    pub column_names: Vec<String>,

    pub field_type: Vec<FieldType>,

    pub auto_inc_counter: i64,

}
