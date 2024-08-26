project(shardkv)

## build eraftkv
set(eraftkv_sources)
list(APPEND eraftkv_sources src/eraftkv_server.cc)
list(APPEND eraftkv_sources src/rocksdb_storage_impl.cc)
list(APPEND eraftkv_sources src/rocksdb_log_storage_impl.cc)
list(APPEND eraftkv_sources src/sequential_file_reader.cc)
list(APPEND eraftkv_sources src/sequential_file_writer.cc)
list(APPEND eraftkv_sources src/log_entry_cache.cc)
list(APPEND eraftkv_sources src/grpc_network_impl.cc)
list(APPEND eraftkv_sources src/client.cc)
list(APPEND eraftkv_sources src/eraftkv.cc)

add_executable(eraftkv ${eraftkv_sources})
target_link_libraries(eraftkv PUBLIC
    rocksdb
    gRPC::grpc++
    ${Protobuf_LIBRARY}
    stdc++fs
    gflags
    eraftcore
)

target_include_directories(eraftkv PUBLIC ${PROJECT_SOURCE_DIR}/include)

## build eraftmeta
set(eraftmeta_sources)
list(APPEND eraftmeta_sources src/eraftkv_server.cc)
list(APPEND eraftmeta_sources src/rocksdb_storage_impl.cc)
list(APPEND eraftmeta_sources src/rocksdb_log_storage_impl.cc)
list(APPEND eraftmeta_sources src/sequential_file_reader.cc)
list(APPEND eraftmeta_sources src/sequential_file_writer.cc)
list(APPEND eraftmeta_sources src/log_entry_cache.cc)
list(APPEND eraftmeta_sources src/grpc_network_impl.cc)
list(APPEND eraftmeta_sources src/client.cc)
list(APPEND eraftmeta_sources src/eraftmeta.cc)

add_executable(eraftmeta ${eraftmeta_sources})
target_link_libraries(eraftmeta PUBLIC
    rocksdb
    gRPC::grpc++
    ${Protobuf_LIBRARY}
    stdc++fs
    gflags
    eraftcore
)
target_include_directories(eraftmeta PUBLIC ${PROJECT_SOURCE_DIR}/include)

## eraft-ctl
add_executable(eraftkv-ctl 
    src/eraftkv_ctl.cc
    src/client.cc
    src/sequential_file_reader.cc
    src/sequential_file_writer.cc
)

target_link_libraries(eraftkv-ctl PUBLIC
    gRPC::grpc++
    ${Protobuf_LIBRARY}
    eraftcore
    protocol
)
target_include_directories(eraftkv-ctl PUBLIC ${PROJECT_SOURCE_DIR}/include)