//! Minimal Redis client implementation
//!
//! Provides an async connect and methods for issuing the supported commands.

use crate::cmd::{Get, Set};
use crate::{connection, Connection, Frame};

use async_stream::try_stream;
use bytes::Bytes;
use std::io::{Error, ErrorKind};
use std::time::Duration;
use tokio::net::{TcpStream, ToSocketAddrs};
use tokio::sync::oneshot::channel;
use tokio_stream::Stream;
use tracing::{debug, instrument};

pub struct Client {
    connection: Connection,
}

#[derive(Debug, Clone)]
pub struct Message {
    pub channel: String,
    pub content: Bytes,
}

// pub async fn connect<T: ToSocketAddrs>(addr: T) -> crate::Result<Client> {
//     // TODO:
// }

impl Client {
    // #[instrument(skip(self))]
    // pub async fn get(&mut self, key: &str) -> crate::Result<Option<Bytes>> {
    //     // TODO:
    // }

    // #[instrument(skip(self))]
    // pub async fn set(&mut self, key: &str, value: Bytes) -> crate::Result<()> {
    //     // TODO:
    // }

    // pub async fn set_expires(
    //     &mut self,
    //     key: &str,
    //     value: Bytes,
    //     expiration: Duration,
    // ) -> crate::Result<()> {
    //     // TODO:
    // }

    // async fn set_cmd(&mut self, cmd: Set) -> crate::Result<()> {
    //     // TODO:
    // }

    // async fn read_response(&mut self) -> crate::Result<Frame> {
    //     // TODO:
    // }
}
