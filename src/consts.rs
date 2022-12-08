
pub static DBS_KEY_PREFIX: &'static str = "/catalog/db/";

pub static TBS_KEY_PREFIX: &'static str = "/catalog/tab/";

pub enum ErrorCode {
    ErrorNone,
    ErrorWrongLeader,
    ErrorTimeOut,
}