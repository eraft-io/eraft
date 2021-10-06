use tokio::sync::broadcast;

#[derive(Debug)]
pub(crate) struct Shutdown {
    shutdown: bool,

    notify: broadcast::Receiver<()>,
}

impl Shutdown {
    pub(crate) fn new(notify: broadcast::Receiver<()>) -> Shutdown {
        Shutdown {
            shutdown: false,
            notify,
        }
    }

    // returns `true` if the shutdown signal has been received.
    pub(crate) fn is_shutdown(&self) -> bool {
        self.shutdown
    }

    /// receive the shutdown notice, waiting if necessary.
    pub(crate) async fn recv(&mut self) {
        if self.shutdown {
            return;
        }

        let _ = self.notify.recv().await;

        self.shutdown = true;
    }
}
