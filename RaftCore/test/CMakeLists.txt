# CMakeLists.txt for UriTests
# 
# © 2020 by LiuJ

cmake_minimum_required(VERSION 3.8)
set(This RaftTests)

set(Sources 
    src/RaftPaperTests.cc
    src/RaftTests.cc
)

add_executable(${This} ${Sources})
set_target_properties(${This} PROPERTIES
    FOLDER Tests
)

target_link_libraries(${This} PUBLIC
    Protocol
    RaftCore
)

add_test(
    NAME ${This}
    COMMAND ${This}
)
