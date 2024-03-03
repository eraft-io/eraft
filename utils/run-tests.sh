#!/bin/bash
set -xe

# run test exe
/eraft/build/rocksdb_storage_impl_tests
/eraft/build/log_entry_cache_tests
/eraft/build/log_entry_cache_benchmark
