use crate::client::Client;
use crate::Result;

use bytes::{Buf, Bytes};
use tokio::sync::mpsc::{channel, Receiver, Sender};
use tokio::sync::oneshot;

// pub fn buffer(client: Client) -> Buffer {
//     // TODO
// }

#[derive(Debug)]
enum Command {
    Get(String),
    Set(String, Bytes),
}

type Message = (Command, oneshot::Sender<Result<Option<Bytes>>>);

// async fn run(mut client: Client, mut rx: Receiver<Message>) {
//     // TODO:
// }

#[derive(Clone)]
pub struct Buffer {
    tx: Sender<Message>,
}

impl Buffer {
    // pub async fn get(&mut self, key: &str) -> Result<Option<Bytes>> {
    //     // TODO:
    // }

    // pub async fn set(&mut self, key: &str, value: Bytes) -> Result<()> {
    //     // TODO:
    // }
}
