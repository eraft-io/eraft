# CMakeLists.txt for Logger
# 
# © 2020 by LiuJ

cmake_minimum_required(VERSION 3.8)
set(This Protocol)

file(GLOB __tmp RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} eraftio/*.cc eraftio/*.h)
list(APPEND generated_files ${__tmp})

add_library(Protocol ${generated_files})
include_directories(${CMAKE_CURRENT_LIST_DIR}/src)
target_include_directories(Protocol PUBLIC ./)
target_link_libraries(${This} PUBLIC
    protobuf
)
