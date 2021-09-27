mod get;
pub use get::Get;

mod set;
pub use set::Set;

mod unknown;
pub use unknown::Unknown;

use crate::{shutdown, Connection, Db, Frame, Parse, ParseError, Shutdown};

#[derive(Debug)]
pub enum Command {
    Get(Get),
    Set(Set),
    Unknown(Unknown),
}

impl Command {
    // pub fn from_frame(frame: Frame) -> crate::Result<Command> {
    //     // TODO:
    // }

    // pub(crate) async fn apply(
    //     self,
    //     db: &Db,
    //     dst: &mut Connection,
    //     shutdown: &mut Shutdown,
    // ) -> crate::Result<()> {
    //     // TODO:
    // }

    // pub(crate) fn get_name(&self) -> &str {
    //     // TODO:
    // }
}
