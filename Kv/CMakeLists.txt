# CMakeLists.txt for Logger
# 
# © 2020 by LiuJ

cmake_minimum_required(VERSION 3.8)
set(This Kv)

set(Headers 
    include/Kv/ServerImpl.h
)

set(Sources
    server/ServerImpl.cc
)

add_library(${This} STATIC ${Sources} ${Headers})
set_target_properties(${This} PROPERTIES
    FOLDER Libraries
)

target_include_directories(${This} PUBLIC include)

target_link_libraries(${This} PUBLIC
    gtest_main
    Logger
    gRPC::grpc++
    Protocol
)

add_subdirectory(test_cli)
add_subdirectory(test_svr)
add_subdirectory(test_kvserver)
add_subdirectory(test_kvcli)
