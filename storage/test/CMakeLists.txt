# CMakeLists.txt for pmem_engine_tests
# 
# © 2022 by eraft dev group

cmake_minimum_required(VERSION 3.8)
set(This storage_test)

set(Sources 
    src/pmem_engine_tests.cc
)

add_executable(${This} ${Sources})
set_target_properties(${This} PROPERTIES
    FOLDER Tests
)

target_link_libraries(${This} PUBLIC
    storage
)

add_test(
    NAME ${This}
    COMMAND ${This}
)
