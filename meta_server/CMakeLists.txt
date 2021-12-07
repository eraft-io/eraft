# CMakeLists.txt for UriTests
# 
# © 2020 by LiuJ

cmake_minimum_required(VERSION 3.8)
set(This e_meta)

set(Sources 
    meta_server.cc
)

add_executable(${This} ${Sources})
set_target_properties(${This} PROPERTIES
    FOLDER Tests
)

target_link_libraries(${This} PUBLIC
    gtest
    gRPC::grpc++
    rocksdb
    snappy
    bz2
    zstd
    lz4
    z
    protocol
    kv
)

add_test(
    NAME ${This}
    COMMAND ${This}
)
