# CMakeLists.txt for UriTests
# 
# © 2020 by LiuJ

cmake_minimum_required(VERSION 3.8)
set(This TestsCli)

set(Sources 
    kv_cli.cc
)

add_executable(${This} ${Sources})
set_target_properties(${This} PROPERTIES
    FOLDER Tests
)

target_link_libraries(${This} PUBLIC
    gtest_main
    Logger
    Kv
    gRPC::grpc++
    Protocol
)

add_test(
    NAME ${This}
    COMMAND ${This}
)
