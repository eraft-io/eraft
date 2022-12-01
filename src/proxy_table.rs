
const MAX_COLUMN_NUM: usize = 100;

pub enum FieldType {
    
    TypeInt,

    TypeFloat,

    TypeVarchar,

    TypeDate,

}

pub trait TabOperation {

    fn create_idx(column_name: &String) -> bool;

    fn drop_idx(column_name: &String) -> bool;

    // insert record to table (feild_name, record_value)
    fn insert_record(record: Vec<(String, String)>) -> bool;

    fn delete_record(match_condition: (String, String)) -> u32;

    fn select_records(query_condition: (String, String)) -> String;

}

pub struct Table {
    
    // the number of table columns
    columns_num: u16,

    records_num: u64,

    name: String,
    
    column_names: [String; MAX_COLUMN_NUM],

    field_type: [FieldType; MAX_COLUMN_NUM],

    auto_inc_counter: i64,

}
