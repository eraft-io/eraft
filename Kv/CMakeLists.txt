# CMakeLists.txt for Logger
# 
# © 2020 by LiuJ

cmake_minimum_required(VERSION 3.8)
set(This Kv)

set(Headers 
    include/Kv/ServerImpl.h
)

set(Sources
    src/ServerImpl.cc
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

add_subdirectory(cli)
add_subdirectory(svr)
add_subdirectory(kvserver)
add_subdirectory(kvcli)
