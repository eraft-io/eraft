# CMakeLists.txt for UriTests
# 
# © 2020 by LiuJ

cmake_minimum_required(VERSION 3.8)
set(This raft_tests)

set(Sources 
    src/raft_paper_tests.cc
    src/raft_tests.cc
)

add_executable(${This} ${Sources})
set_target_properties(${This} PROPERTIES
    FOLDER Tests
)

target_link_libraries(${This} PUBLIC
    protocol
    raftcore
)

add_test(
    NAME ${This}
    COMMAND ${This}
)
