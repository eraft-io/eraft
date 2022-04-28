# MIT License

# Copyright (c) 2021 eraft group

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.8)
set(This eraft)
project(${This} C CXX)

set(ERAFT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_C_STANDARD 99)
set(CXX_STANDARD 17 CACHE STRING "C++ language standard")

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD ${CXX_STANDARD})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Google Test
set(BUILD_GTEST ON CACHE BOOL "Builds the googletest subproject" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "Builds the googlemock subproject" FORCE)

add_subdirectory(googletest)
set_target_properties(gtest_main gtest PROPERTIES
    FOLDER libraries
)

add_subdirectory(protocol)
add_subdirectory(raftcore)
add_subdirectory(network)
add_subdirectory(storage)
add_subdirectory(third_party/libredis)
add_subdirectory(cmd)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
