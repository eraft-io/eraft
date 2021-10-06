pub mod blocking_client;
pub mod client;

pub mod frame;
pub use frame::Frame;

pub mod cmd;

mod buffer;

mod eraft_kv;

mod db;
use db::Db;
use db::DbDropGuard;

mod parse;
use parse::{Parse, ParseError};

mod connection;
pub use connection::Connection;

pub const DEFAULT_PORT: &str = "6379";

pub type Error = Box<dyn std::error::Error + Send + Sync>;

pub type Result<T> = std::result::Result<T, Error>;

mod shutdown;
use shutdown::Shutdown;
