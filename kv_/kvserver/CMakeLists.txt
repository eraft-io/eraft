# CMakeLists.txt for UriTests
# 
# © 2020 by LiuJ

cmake_minimum_required(VERSION 3.8)
set(This kv_svr)

set(Sources 
    main_server.cc
)

add_executable(${This} ${Sources})
set_target_properties(${This} PROPERTIES
    FOLDER Tests
)

target_link_libraries(${This} PUBLIC
    gtest
    gRPC::grpc++
    rocksdb
    protocol
    kv
)

add_test(
    NAME ${This}
    COMMAND ${This}
)
