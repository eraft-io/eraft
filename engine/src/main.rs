//! simplekv.rs -- a rust implementation of [simplekv example] from PMDK,
//! libpmemobj++ of simple kv which uses vector to hold values, string as a key
//! and array to hold buckets.
//!
//! [simplekv example]: https://github.com/pmem/libpmemobj-cpp/tree/master/examples/simplekv
#![feature(type_name_of_val)]

use corundum::default::*;
use corundum::open_flags::*;

type P = Allocator;

pub fn main() {
    use std::env;
    use std::vec::Vec as StdVec;

    let args: StdVec<String> = env::args().collect();

    if args.len() < 3 {
        println!(
            "usage: {} file-name [get key|put key value] | [burst get|put|putget count]",
            args[0]
        );
        return;
    }

    let root = P::open::<pmem_wrapper::kv::KvStore<i32>>(&args[1], O_CFNE | O_1GB).unwrap();

    if args[2] == String::from("get") && args.len() == 4 {
        println!("{:?}", root.get(&*args[3]))
    } else if args[2] == String::from("put") && args.len() == 5 {
        root.put(&*args[3], args[4].parse().unwrap())
    }
    if args[2] == String::from("burst")
        && (args[3] == String::from("put") || args[3] == String::from("putget"))
        && args.len() == 5
    {
        for i in 0..args[4].parse().unwrap() {
            let key = format!("key{}", i);
            root.put(&*key, i);
        }
    }
    if args[2] == String::from("burst")
        && (args[3] == String::from("get") || args[3] == String::from("putget"))
        && args.len() == 5
    {
        for i in 0..args[4].parse().unwrap() {
            let key = format!("key{}", i);
            let value = root.get(&*key).unwrap();
            println!("Got value: {}", value);
        }
    }
}
