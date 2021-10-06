use crate::{Connection, Frame};
use tracing::{debug, instrument};

#[derive(Debug)]
pub struct Unknown {
    command_name: String,
}

impl Unknown {
    // pub(crate) fn new(key: impl ToString) -> Unknown {
    //     // TODO:
    // }

    // pub(crate) fn get_name(&self) -> &str {
    //     // TODO:
    // }

    // pub(crate) async fn apply(self, dst: &mut Connection) -> crate::Result<()> {
    //     // TODO:
    // }
}
