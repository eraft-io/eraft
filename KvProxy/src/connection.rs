use crate::frame::{self, Frame};

use bytes::{Buf, BytesMut};
use std::io::{self, Cursor};
use tokio::io::{AsyncReadExt, AsyncWriteExt, BufWriter};
use tokio::net::TcpStream;

pub struct Connection {
    // 1.TcpStream 2.buffer for reading frames
}

impl Connection {
    pub fn new(socket: TcpStream) -> Connection {
        Connection {}
    }

    pub async fn read_frame(&mut self) -> crate::Result<Option<Frame>> {
        loop {}
    }

    // fn parse_frame(&mut self) -> crate::Result<Option<Frame>> {
    // TODO:
    // }

    // pub async fn write_frame(&mut self, frame: &Frame) -> io::Result<()> {
    // TODO:
    // }

    // async fn write_value(&mut self, frame: &Frame) -> io::Result<()> {
    // TODO:
    // }

    // async fn write_decimal(&mut self, val: u64) -> io::Result<()> {
    // TODO:
    // }
}
