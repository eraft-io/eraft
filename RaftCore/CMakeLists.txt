# CMakeLists.txt for RaftCore
# 
# © 2020 by LiuJ

cmake_minimum_required(VERSION 3.8)
set(This RaftCore)

set(Headers 
    include/RaftCore/Log.h
    include/RaftCore/MemoryStorage.h
    include/RaftCore/Raft.h
    include/RaftCore/RawNode.h
    include/RaftCore/Storage.h
    include/RaftCore/Util.h
)

set(Sources
    src/Log.cc
    src/MemoryStorage.cc
    src/Raft.cc
    src/RawNode.cc
)

add_library(${This} STATIC ${Sources} ${Headers})
set_target_properties(${This} PROPERTIES
    FOLDER Libraries
)

target_include_directories(${This} PUBLIC include)

target_link_libraries(${This} PUBLIC
    Protocol
    Logger
    gtest_main
)

add_subdirectory(test)
