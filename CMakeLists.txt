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
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(${ERAFT_ROOT_DIR}/cmake/helpers.cmake)
set_version(VERSION)

# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2017-2021, Intel Corporation

# ----------------------------------------------------------------- #
## CMake build options
# ----------------------------------------------------------------- #
option(BUILD_DOC "build documentation" ON)
option(BUILD_EXAMPLES "build examples" ON)
option(BUILD_TESTS "build tests" ON)
option(BUILD_JSON_CONFIG "build the 'libpmemkv_json_config' library" ON)

option(TESTS_LONG "enable long running tests" OFF)
option(TESTS_USE_FORCED_PMEM "run tests with PMEM_IS_PMEM_FORCE=1 - it speeds up tests execution on emulated pmem" OFF)
option(TESTS_USE_VALGRIND "enable tests with valgrind (fail build if Valgrind not found)" ON)
option(TESTS_PMEMOBJ_DRD_HELGRIND "enable test of pmemobj engines under drd and helgrind (they should only be run on PMEM)" OFF)
option(TESTS_JSON "enable tests which require libpmemkv_json_config library (BUILD_JSON_CONFIG has to be ON)" ON)

option(COVERAGE "enable collecting of coverage data" OFF)
option(DEVELOPER_MODE "enable developer's checks" OFF)
option(CHECK_CPP_STYLE "check code style of C++ sources" OFF)
option(USE_ASAN "enable AddressSanitizer (debugging)" OFF)
option(USE_UBSAN "enable UndefinedBehaviorSanitizer (debugging)" OFF)
option(USE_CCACHE "use ccache if it is available in the system" ON)

# Each engine can be enabled separately.
option(ENGINE_CMAP "enable cmap engine" ON)
option(ENGINE_VCMAP "enable vcmap engine" ON)
option(ENGINE_VSMAP "enable vsmap engine" ON)
option(ENGINE_CSMAP "enable experimental csmap engine (requires CXX_STANDARD to be set to value >= 14)" OFF)
option(ENGINE_STREE "enable experimental stree engine" ON)
option(ENGINE_TREE3 "enable experimental tree3 engine" OFF)
option(ENGINE_RADIX "enable experimental radix engine" OFF)
option(ENGINE_ROBINHOOD "enable experimental robinhood engine (requires CXX_STANDARD to be set to value >= 14)" OFF)
option(ENGINE_DRAM_VCMAP "enable testing dram_vcmap engine" OFF)

# ----------------------------------------------------------------- #
## Set required and useful variables
# ----------------------------------------------------------------- #
set(CXX_STANDARD 17 CACHE STRING "C++ language standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD ${CXX_STANDARD})

# Specify and print the build type
set(DEFAULT_BUILD_TYPE "RelWithDebInfo")
set(predefined_build_types
	Debug
	Release
	RelWithDebInfo
	MinSizeRel)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE ${DEFAULT_BUILD_TYPE}
		CACHE STRING "choose the type of build (${predefined_build_types})" FORCE)
	message(STATUS "CMAKE_BUILD_TYPE not set, setting the default one: ${CMAKE_BUILD_TYPE}")
else()
	message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
	if(NOT CMAKE_BUILD_TYPE IN_LIST predefined_build_types)
		message(WARNING "Unusual build type was set, please make sure it's proper one. "
			"By default supported are only following: ${predefined_build_types}.")
	endif()
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ERAFT_ROOT_DIR}/cmake)
set(LIBPMEMOBJ_CPP_REQUIRED_VERSION 1.13.0)
set(LIBPMEMOBJ_REQUIRED_VERSION 1.9)
set(LIBPMEM_REQUIRED_VERSION 1.7)
set(MEMKIND_REQUIRED_VERSION 1.8.0)
set(RAPIDJSON_REQUIRED_VERSION 1.0.0)

set(PKG_CONFIG_REQUIRES)
set(DEB_DEPENDS)
set(RPM_DEPENDS)

set(TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/test CACHE STRING "working directory for tests")
message(STATUS "TEST_DIR set to: \"${TEST_DIR}\"")

# Do not treat include directories from the interfaces
# of consumed Imported Targets as SYSTEM by default.
set(CMAKE_NO_SYSTEM_FROM_IMPORTED 1)

set(SOURCE_FILES
	storage/pmemkv/libpmemkv.cc
	storage/pmemkv/libpmemkv.h
	storage/pmemkv/engine.cc
	storage/pmemkv/engines/blackhole.cc
	storage/pmemkv/engines/blackhole.h
	storage/pmemkv/out.cc
	storage/pmemkv/out.h
	storage/pmemkv/iterator.h
	storage/pmemkv/iterator.cc
)
# Add each engine source separately
if(ENGINE_CMAP)
	list(APPEND SOURCE_FILES
        storage/pmemkv/engines/cmap.h
		storage/pmemkv/engines/cmap.cc
	)
endif()
if(ENGINE_CSMAP)
	list(APPEND SOURCE_FILES
        storage/pmemkv/engines-experimental/csmap.h
        storage/pmemkv/engines-experimental/csmap.cc
	)
endif()
if(ENGINE_VCMAP)
	list(APPEND SOURCE_FILES
        storage/pmemkv/engines/basic_vcmap.h
        storage/pmemkv/engines/vcmap.h
        storage/pmemkv/engines/vcmap.cc
	)
endif()
if(ENGINE_VSMAP)
	list(APPEND SOURCE_FILES
        storage/pmemkv/engines/vsmap.h
        storage/pmemkv/engines/vsmap.cc
	)
endif()
if(ENGINE_STREE)
	list(APPEND SOURCE_FILES
        storage/pmemkv/engines-experimental/stree.h
        storage/pmemkv/engines-experimental/stree.cc
        storage/pmemkv/engines-experimental/stree/persistent_b_tree.h
	)
endif()
if(ENGINE_TREE3)
	list(APPEND SOURCE_FILES
        storage/pmemkv/engines-experimental/tree3.h
		storage/pmemkv/engines-experimental/tree3.cc
	)
endif()
if(ENGINE_RADIX)
	list(APPEND SOURCE_FILES
        storage/pmemkv/engines-experimental/radix.h
		storage/pmemkv/engines-experimental/radix.cc
	)
endif()
if(ENGINE_ROBINHOOD)
	list(APPEND SOURCE_FILES
        storage/pmemkv/engines-experimental/robinhood.h
        storage/pmemkv/engines-experimental/robinhood.cc
		storage/pmemkv/fast_hash.h
		storage/pmemkv/fast_hash.cc
	)
endif()
if(ENGINE_DRAM_VCMAP)
	list(APPEND SOURCE_FILES
        storage/pmemkv/engines/basic_vcmap.h
		storage/pmemkv/engines-testing/dram_vcmap.h
		storage/pmemkv/engines-testing/dram_vcmap.cc
	)
endif()


# ----------------------------------------------------------------- #
## Setup defines and check status of each engine
# ----------------------------------------------------------------- #
if(ENGINE_CMAP)
	add_definitions(-DENGINE_CMAP)
	message(STATUS "CMAP engine is ON")
else()
	message(STATUS "CMAP engine is OFF")
endif()
if(ENGINE_CSMAP)
	add_definitions(-DENGINE_CSMAP)
	message(STATUS "CSMAP engine is ON")

	if(CXX_STANDARD LESS 14)
		message(FATAL_ERROR "CXX_STANDARD must be >= 14 if ENGINE_CSMAP is ON")
	endif()
else()
	message(STATUS "CSMAP engine is OFF")
endif()
if(ENGINE_VCMAP)
	add_definitions(-DENGINE_VCMAP)
	message(STATUS "VCMAP engine is ON")
else()
	message(STATUS "VCMAP engine is OFF")
endif()
if(ENGINE_VSMAP)
	add_definitions(-DENGINE_VSMAP)
	message(STATUS "VSMAP engine is ON")
else()
	message(STATUS "VSMAP engine is OFF")
endif()
if(ENGINE_STREE)
	add_definitions(-DENGINE_STREE)
	message(STATUS "STREE engine is ON")
else()
	message(STATUS "STREE engine is OFF")
endif()
if(ENGINE_TREE3)
	add_definitions(-DENGINE_TREE3)
	message(STATUS "TREE3 engine is ON")
else()
	message(STATUS "TREE3 engine is OFF")
endif()
if(ENGINE_RADIX)
	add_definitions(-DENGINE_RADIX)
	message(STATUS "RADIX engine is ON")
else()
	message(STATUS "RADIX engine is OFF")
endif()
if(ENGINE_ROBINHOOD)
	add_definitions(-DENGINE_ROBINHOOD)
	message(STATUS "ROBINHOOD engine is ON")

	if(CXX_STANDARD LESS 14)
		message(FATAL_ERROR "CXX_STANDARD must be >= 14 if ENGINE_ROBINHOOD is ON")
	endif()
else()
	message(STATUS "ROBINHOOD engine is OFF")
endif()
if(ENGINE_DRAM_VCMAP)
	add_definitions(-DENGINE_DRAM_VCMAP)
	message(STATUS "DRAM_VCMAP engine is ON")
else()
	message(STATUS "DRAM_VCMAP engine is OFF")
endif()

# ----------------------------------------------------------------- #
## Set compiler's flags
# ----------------------------------------------------------------- #
if(DEVELOPER_MODE)
	add_common_flag(-Werror)
endif()
if(USE_ASAN)
	add_sanitizer_flag(address)
endif()
if(USE_UBSAN)
	add_sanitizer_flag(undefined)
endif()
if(COVERAGE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -coverage")
endif()

add_common_flag(-Wall)
add_common_flag(-Wpointer-arith)
add_common_flag(-Wsign-compare)
add_common_flag(-Wunreachable-code-return)
add_common_flag(-Wmissing-variable-declarations)
add_common_flag(-fno-common)
add_common_flag(-Wunused-macros)
add_common_flag(-Wsign-conversion)
add_common_flag(-Wno-maybe-uninitialized)

add_common_flag(-ggdb DEBUG)
add_common_flag(-DDEBUG DEBUG)

add_common_flag("-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2" RELEASE)

# ----------------------------------------------------------------- #
## Setup environment, find packages,
## configure list of package dependencies
# ----------------------------------------------------------------- #
find_package(PkgConfig QUIET)
include(FindPerl)
include(ExternalProject)
include(FindThreads)
include(CheckCXXSourceCompiles)
include(GNUInstallDirs)

include(libpmemobj++)
list(APPEND PKG_CONFIG_REQUIRES "libpmemobj++ >= ${LIBPMEMOBJ_CPP_REQUIRED_VERSION}")
list(APPEND RPM_DEPENDS "libpmemobj >= ${LIBPMEMOBJ_REQUIRED_VERSION}")
list(APPEND DEB_DEPENDS "libpmemobj1 (>= ${LIBPMEMOBJ_REQUIRED_VERSION}) | libpmemobj (>= ${LIBPMEMOBJ_REQUIRED_VERSION})")

# Configure the ccache as compiler launcher
find_program(CCACHE_FOUND ccache)
if(USE_CCACHE AND CCACHE_FOUND)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
endif()

if(ENGINE_VSMAP OR ENGINE_VCMAP)
	include(memkind)
	list(APPEND PKG_CONFIG_REQUIRES "memkind >= ${MEMKIND_REQUIRED_VERSION}")
	list(APPEND RPM_DEPENDS "memkind >= ${MEMKIND_REQUIRED_VERSION}")
	list(APPEND DEB_DEPENDS "libmemkind0 (>= ${MEMKIND_REQUIRED_VERSION})")
endif()

if(ENGINE_VCMAP OR ENGINE_DRAM_VCMAP)
	include(tbb)
	add_definitions(-DTBB_DEFINE_STD_HASH_SPECIALIZATIONS)
	list(APPEND PKG_CONFIG_REQUIRES tbb)
	list(APPEND RPM_DEPENDS tbb)
	list(APPEND DEB_DEPENDS libtbb2)
endif()

# ----------------------------------------------------------------- #
## Link libraries, setup targets
# ----------------------------------------------------------------- #
add_library(pmemkv SHARED ${SOURCE_FILES})
set_target_properties(pmemkv PROPERTIES SOVERSION 1)
target_link_libraries(pmemkv PRIVATE
	-Wl,--version-script=${ERAFT_ROOT_DIR}/storage/pmemkv/libpmemkv.map)

target_include_directories(pmemkv PRIVATE storage/pmemkv/valgrind)
# Enable libpmemobj-cpp valgrind annotations
target_compile_options(pmemkv PRIVATE -DLIBPMEMOBJ_CPP_VG_ENABLED=1)

target_link_libraries(pmemkv PRIVATE ${LIBPMEMOBJ++_LIBRARIES})
if(ENGINE_VSMAP OR ENGINE_VCMAP)
	target_link_libraries(pmemkv PRIVATE ${MEMKIND_LIBRARIES})
endif()
if(ENGINE_VCMAP OR ENGINE_DRAM_VCMAP)
	target_link_libraries(pmemkv PRIVATE ${TBB_LIBRARIES})
endif()

# ----------------------------------------------------------------- #
## Add sub-directories if builds enabled
# ----------------------------------------------------------------- #
if(BUILD_JSON_CONFIG)
	include(rapidjson)
	list(APPEND PKG_CONFIG_REQUIRES "RapidJSON >= ${RAPIDJSON_REQUIRED_VERSION}")
	list(APPEND RPM_DEPENDS "rapidjson-devel >= ${RAPIDJSON_REQUIRED_VERSION}")
	list(APPEND DEB_DEPENDS "rapidjson-dev (>= ${RAPIDJSON_REQUIRED_VERSION})")
	add_definitions(-DBUILD_JSON_CONFIG)

	set(SOURCE_FILES_JSON_CONFIG
        storage/pmemkv/libpmemkv_json_config.cc
        storage/pmemkv/libpmemkv_json_config.h
        storage/pmemkv/out.cc
        storage/pmemkv/out.h
	)

	add_library(pmemkv_json_config SHARED ${SOURCE_FILES_JSON_CONFIG})
	set_target_properties(pmemkv_json_config PROPERTIES SOVERSION 1)
	target_link_libraries(pmemkv_json_config PRIVATE pmemkv ${RapidJSON_LIBRARIES}
		-Wl,--version-script=${ERAFT_ROOT_DIR}/storage/pmemkv/libpmemkv_json_config.map)
	set_target_properties(pmemkv_json_config
		PROPERTIES PUBLIC_HEADER "src/libpmemkv_json_config.h")

	configure_file(libpmemkv_json_config.pc.in libpmemkv_json_config.pc @ONLY)

	install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libpmemkv_json_config.pc
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

	install(TARGETS pmemkv_json_config
		PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

# Orgnize application, libraries, etc. in floders in the IDE.
set_property(GLOBAL PROPERTY USE_FLODERS ON)

# Ensure proper loading of side-loaded shared libraries on all target
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
if(APPLE)
  set(CMAKE_INSTALL_RPATH "@loader_path")
elseif(UNIX)
  set(CMAKE_INSTALL_RPATH "\$ORIGIN")
endif()

# Don't add the goofy "d" postfix for debug builds of libraries.
set(CMAKE_DEBUG_POSTFIX "")

# Suppress Microsoft warnings about functions being unsafe.
add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)

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

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
