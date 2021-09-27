use bytes::Bytes;
use std::time::Duration;
use tokio::net::ToSocketAddrs;
use tokio::runtime::Runtime;

pub use crate::client::Message;

pub struct BlockingClient {
    inner: crate::client::Client,

    rt: Runtime,
}

// pub fn connect<T: ToSocketAddrs>(addr: T) -> crate::Result<BlockingClient> {
//     // TODO:
// }

impl BlockingClient {
    // / Get the value of key.
    // /
    // / If the key does not exist the special value `None` is returned.
    // /
    // / # Examples
    // /
    // / Demonstrates basic usage.
    // /
    // / ```no_run
    // / use mini_redis::blocking_client;
    // /
    // / fn main() {
    // /     let mut client = blocking_client::connect("localhost:6379").unwrap();
    // /
    // /     let val = client.get("foo").unwrap();
    // /     println!("Got = {:?}", val);
    // / }
    // / ```
    // pub fn get(&mut self, key: &str) -> crate::Result<Option<Bytes>> {
    //     // TODO:
    // }

    // / Set `key` to hold the given `value`.
    // /
    // / The `value` is associated with `key` until it is overwritten by the next
    // / call to `set` or it is removed.
    // /
    // / If key already holds a value, it is overwritten. Any previous time to
    // / live associated with the key is discarded on successful SET operation.
    // /
    // / # Examples
    // /
    // / Demonstrates basic usage.
    // /
    // / ```no_run
    // / use mini_redis::blocking_client;
    // /
    // / fn main() {
    // /     let mut client = blocking_client::connect("localhost:6379").unwrap();
    // /
    // /     client.set("foo", "bar".into()).unwrap();
    // /
    // /     // Getting the value immediately works
    // /     let val = client.get("foo").unwrap().unwrap();
    // /     assert_eq!(val, "bar");
    // / }
    // / ```

    // pub fn set(&mut self, key: &str, value: Bytes) -> crate::Result<()> {
    //     // TODO:
    // }

    // / Set `key` to hold the given `value`. The value expires after `expiration`
    // /
    // / The `value` is associated with `key` until one of the following:
    // / - it expires.
    // / - it is overwritten by the next call to `set`.
    // / - it is removed.
    // /
    // / If key already holds a value, it is overwritten. Any previous time to
    // / live associated with the key is discarded on a successful SET operation.
    // /
    // / # Examples
    // /
    // / Demonstrates basic usage. This example is not **guaranteed** to always
    // / work as it relies on time based logic and assumes the client and server
    // / stay relatively synchronized in time. The real world tends to not be so
    // / favorable.
    // /
    // / ```no_run
    // / use mini_redis::blocking_client;
    // / use std::thread;
    // / use std::time::Duration;
    // /
    // / fn main() {
    // /     let ttl = Duration::from_millis(500);
    // /     let mut client = blocking_client::connect("localhost:6379").unwrap();
    // /
    // /     client.set_expires("foo", "bar".into(), ttl).unwrap();
    // /
    // /     // Getting the value immediately works
    // /     let val = client.get("foo").unwrap().unwrap();
    // /     assert_eq!(val, "bar");
    // /
    // /     // Wait for the TTL to expire
    // /     thread::sleep(ttl);
    // /
    // /     let val = client.get("foo").unwrap();
    // /     assert!(val.is_some());
    // / }
    // / ```

    // pub fn set_expires(
    //     &mut self,
    //     key: &str,
    //     value: Bytes,
    //     expiration: Duration,
    // ) -> crate::Result<()> {
    //     // TODO:
    // }
}
