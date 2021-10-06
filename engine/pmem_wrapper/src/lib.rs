#![feature(test)]

extern crate test;

pub mod kv;


#[cfg(test)]
mod tests {
  use corundum::open_flags::*;
  use corundum::default::*;
  use test::Bencher;

  #[bench]
  fn bench_put_key(b: &mut Bencher) {
    let root = Allocator::open::<crate::kv::KvStore<i32>>("testfile", O_CFNE | O_1GB).unwrap();
    b.iter(move || {
      root.put("testkey", 1213);
    });
  }
}
