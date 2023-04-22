#!/bin/bash
set -xe

# run test exe
/eraft/build/gtest_example_tests
/eraft/build/eraftkv_server_test
/eraft/build/rocksdb_storage_impl_tests
/eraft/build/log_entry_cache_tests
/eraft/build/log_entry_cache_tests
/eraft/build/google_example_banchmark
/eraft/build/log_entry_cache_benchmark
/eraft/build/grpc_network_impl_test
/eraft/build/eraftkv
