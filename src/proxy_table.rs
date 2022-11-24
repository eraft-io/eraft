
const MAX_COLUMN_NUM: usize = 100;

pub enum FieldType {
    
    TypeInt,

    TypeFloat,

    TypeVarchar,

    TypeDate,

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



