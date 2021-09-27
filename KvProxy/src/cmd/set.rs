use crate::cmd::{Parse, ParseError};
use crate::{Connection, Db, Frame};
use bytes::Bytes;
use std::time::Duration;
use tracing::{debug, instrument};

#[derive(Debug)]
pub struct Set {
    key: String,

    value: Bytes,

    expire: Option<Duration>,
}

impl Set {
    // pub fn new(key: impl ToString, value: Bytes, expire: Option<Duration>) -> Set {
    //     // TODO:
    // }

    // pub fn key(&self) -> &str {
    //     // TODO:
    // }

    // pub fn value(&self) -> &Bytes {
    //     // TODO:
    // }

    // pub fn expire(&self) -> Option<Duration> {
    //     // TODO:
    // }

    // pub(crate) fn parse_frames(parse: &mut Parse) -> crate::Result<Set> {
    //     // TODO:
    // }

    // #[instrument(skip(self, db, dst))]
    // pub(crate) async fn apply(self, db: &Db, dst: &mut Connection) -> crate::Result<()> {
    //     // TODO:
    // }

    // pub(crate) fn into_frame(self) -> Frame {
    //     // TODO:
    // }
}
