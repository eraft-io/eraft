use tokio::sync::{broadcast, Notify};
use tokio::time::{self, Duration, Instant};

use bytes::Bytes;
use std::collections::{BTreeMap, HashMap};
use std::ops::Mul;
use std::sync::{Arc, Mutex};
use tracing::debug;

#[derive(Debug)]
pub(crate) struct DbDropGuard {
    db: Db,
}

#[derive(Debug, Clone)]
pub(crate) struct Db {
    // shared: Arc<Shared>,
}

#[derive(Debug)]
struct Shared {
    state: Mutex<State>,

    background_task: Notify,
}

#[derive(Debug)]
struct State {
    // the key-value data.
    entries: HashMap<String, Entry>,

    // Tracks key TTLs.
    expirations: BTreeMap<(Instant, u64), String>,

    // Identifier to use for the next expiration.
    next_id: u64,

    /// True when the Db instance is shutting down.
    shutdown: bool,
}

#[derive(Debug)]
struct Entry {
    /// Uniquely identifies this entry.
    id: u64,

    /// Shored data
    data: Bytes,

    /// Instant at which the entry expires and should be removed from the databases.
    expires_at: Option<Instant>,
}

impl DbDropGuard {
    pub(crate) fn new() -> DbDropGuard {
        DbDropGuard { db: Db::new() }
    }

    pub(crate) fn db(&self) -> Db {
        self.db.clone()
    }
}

impl Db {
    // create a new empty db
    pub(crate) fn new() -> Db {
        // TODO:
        Db {}
    }

    // pub(crate) fn get(&self, key: &str) -> Option<Bytes> {
    // TODO:
    // }

    // pub(crate) fn set(&self, key: String, value: Bytes, expire: Option<Duration>) {
    // TODO:
    // }

    // fn shutdown_purge_task(&self) {
    // TODO:
    // }
}

impl Shared {
    // fn purge_expired_keys(&self) -> Option<Instant> {
    // TODO:
    // }

    // fn is_shutdown(&self) -> bool {
    // TODO:
    // }
}

impl State {
    // fn next_expiration(&self) -> Option<Instant> {
    // TODO:
    // }
}

// async fn purge_expired_tasks(shared: Arc<Shared>) {
// TODO:
// }
