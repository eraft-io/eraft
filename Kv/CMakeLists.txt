# CMakeLists.txt for Logger
# 
# © 2020 by LiuJ

cmake_minimum_required(VERSION 3.8)
set(This Kv)

set(Headers 
    include/Kv/ServerImpl.h
    include/Kv/File.h
    include/Kv/Engines.h
)

set(Sources
    server/ServerImpl.cc
    server/File.cc
    server/Engines.cc
)

add_library(${This} STATIC ${Sources} ${Headers})
set_target_properties(${This} PROPERTIES
    FOLDER Libraries
)

target_include_directories(${This} PUBLIC include)

target_link_libraries(${This} PUBLIC
    gtest_main
    Logger
    leveldb
    gRPC::grpc++
    Protocol
    RaftCore
)

add_subdirectory(test_cli)
add_subdirectory(test_svr)
add_subdirectory(test_kvserver)
add_subdirectory(test_kvcli)
