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
