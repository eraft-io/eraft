use crate::{Connection, Db, Frame, Parse};

use bytes::Bytes;
use tracing::{debug, instrument};

#[derive(Debug)]
pub struct Get {
    key: String,
}

impl Get {
    // pub fn new(key: impl ToString) -> Get {
    //     // TODO:
    // }

    // pub fn key(&self) -> &str {
    //     // TODO:
    // }

    // pub(crate) fn parse_frames(parse: &mut Parse) -> crate::Result<Get> {
    //     // TODO:
    // }

    // pub(crate) async fn apply(self, db: &Db, dst: &mut Connection) -> crate::Result<()> {
    //     // TODO:
    // }

    // pub(crate) fn into_frame(self) -> Frame {
    //     // TODO:
    // }
}
