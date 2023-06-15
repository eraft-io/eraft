#!/bin/bash

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
SRCPATH=$(cd $SCRIPTPATH/..; pwd -P)

clang-format -style=file --sort-includes -i ${SRCPATH}/src/eraftkv_server.cc
clang-format -style=file --sort-includes -i ${SRCPATH}/src/eraftkv_server.h
clang-format -style=file --sort-includes -i ${SRCPATH}/src/eraftkv_server_test.cc
clang-format -style=file --sort-includes -i ${SRCPATH}/src/eraftmeta_server.h
clang-format -style=file --sort-includes -i ${SRCPATH}/src/estatus.h
clang-format -style=file --sort-includes -i ${SRCPATH}/src/grpc_network_impl.cc
clang-format -style=file --sort-includes -i ${SRCPATH}/src/grpc_network_impl.h
clang-format -style=file --sort-includes -i ${SRCPATH}/src/grpc_network_impl_test.cc
clang-format -style=file --sort-includes -i ${SRCPATH}/src/log_entry_cache_benchmark.cc
clang-format -style=file --sort-includes -i ${SRCPATH}/src/log_entry_cache.cc
clang-format -style=file --sort-includes -i ${SRCPATH}/src/log_entry_cache.h
clang-format -style=file --sort-includes -i ${SRCPATH}/src/log_entry_cache_tests.cc
clang-format -style=file --sort-includes -i ${SRCPATH}/src/log_storage_impl.cc
clang-format -style=file --sort-includes -i ${SRCPATH}/src/main.cc
clang-format -style=file --sort-includes -i ${SRCPATH}/src/network.h
clang-format -style=file --sort-includes -i ${SRCPATH}/src/raft_config.h
clang-format -style=file --sort-includes -i ${SRCPATH}/src/raft_log.h
clang-format -style=file --sort-includes -i ${SRCPATH}/src/raft_node.h
clang-format -style=file --sort-includes -i ${SRCPATH}/src/raft_server.cc
clang-format -style=file --sort-includes -i ${SRCPATH}/src/raft_server.h
clang-format -style=file --sort-includes -i ${SRCPATH}/src/rocksdb_storage_impl.cc
clang-format -style=file --sort-includes -i ${SRCPATH}/src/rocksdb_storage_impl.h
clang-format -style=file --sort-includes -i ${SRCPATH}/src/rocksdb_storage_impl_tests.cc
clang-format -style=file --sort-includes -i ${SRCPATH}/src/storage.h
clang-format -style=file --sort-includes -i ${SRCPATH}/src/util.cc
clang-format -style=file --sort-includes -i ${SRCPATH}/src/util.h
clang-format -style=file --sort-includes -i ${SRCPATH}/src/eraft_vdb_server.cc
clang-format -style=file --sort-includes -i ${SRCPATH}/src/thread_pool.h
