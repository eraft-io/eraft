cmake_minimum_required(VERSION 3.16)
project(raft_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)

# ── RocksDB ───────────────────────────────────────────────────
find_package(RocksDB QUIET)
if(NOT RocksDB_FOUND)
    # Homebrew installs RocksDB with pkg-config
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(ROCKSDB QUIET rocksdb)
    endif()
endif()

if(RocksDB_FOUND OR ROCKSDB_FOUND)
    message(STATUS "Found RocksDB — enabling KV Raft")
    set(RAFT_HAS_ROCKSDB TRUE)
else()
    message(STATUS "RocksDB not found — KV Raft will not be built")
    set(RAFT_HAS_ROCKSDB FALSE)
endif()

# ── gRPC + Protobuf (optional, system install only) ──────────
# IMPORTANT: find gRPC first — it internally pulls in Protobuf as a
# dependency. Calling find_package(Protobuf) before gRPC causes
# duplicate-target errors with newer protobuf versions.
find_package(gRPC QUIET)

if(gRPC_FOUND)
    message(STATUS "Found system gRPC — enabling gRPC transport")
    set(RAFT_HAS_GRPC TRUE)

    # Locate protoc and grpc_cpp_plugin
    find_program(_PROTOBUF_PROTOC protoc REQUIRED)
    find_program(_GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)
    message(STATUS "  protoc:          ${_PROTOBUF_PROTOC}")
    message(STATUS "  grpc_cpp_plugin: ${_GRPC_CPP_PLUGIN}")

    # Proto generation
    set(PROTO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
    set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto_gen)
    file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

    set(RAFT_PROTO ${PROTO_SRC_DIR}/raft.proto)
    set(RAFT_PROTO_GEN_CC ${PROTO_GEN_DIR}/raft.pb.cc)
    set(RAFT_PROTO_GEN_H  ${PROTO_GEN_DIR}/raft.pb.h)
    set(RAFT_GRPC_GEN_CC  ${PROTO_GEN_DIR}/raft.grpc.pb.cc)
    set(RAFT_GRPC_GEN_H   ${PROTO_GEN_DIR}/raft.grpc.pb.h)

    add_custom_command(
        OUTPUT  ${RAFT_PROTO_GEN_CC} ${RAFT_PROTO_GEN_H}
                ${RAFT_GRPC_GEN_CC}  ${RAFT_GRPC_GEN_H}
        COMMAND ${_PROTOBUF_PROTOC}
            --proto_path=${PROTO_SRC_DIR}
            --cpp_out=${PROTO_GEN_DIR}
            --grpc_out=${PROTO_GEN_DIR}
            --plugin=protoc-gen-grpc=${_GRPC_CPP_PLUGIN}
            ${RAFT_PROTO}
        DEPENDS ${RAFT_PROTO}
        COMMENT "Generating protobuf/gRPC C++ sources"
    )

    add_library(raft_proto STATIC
        ${RAFT_PROTO_GEN_CC}
        ${RAFT_GRPC_GEN_CC}
    )
    target_include_directories(raft_proto PUBLIC ${PROTO_GEN_DIR})
    target_link_libraries(raft_proto PUBLIC gRPC::grpc++)
else()
    message(STATUS "Protobuf/gRPC not found — building core library only (no gRPC transport)")
    set(RAFT_HAS_GRPC FALSE)
endif()

# ── Raft core library ────────────────────────────────────────
set(RAFT_CORE_SRCS
    src/util.cpp
    src/persister.cpp
    src/raft.cpp
)

if(RAFT_HAS_GRPC)
    list(APPEND RAFT_CORE_SRCS
        src/grpc_client.cpp
        src/grpc_server.cpp
    )
endif()

add_library(raft_core STATIC ${RAFT_CORE_SRCS})
target_include_directories(raft_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(raft_core PUBLIC Threads::Threads)

if(RAFT_HAS_GRPC)
    target_include_directories(raft_core PUBLIC ${PROTO_GEN_DIR})
    target_link_libraries(raft_core PUBLIC raft_proto)
    target_compile_definitions(raft_core PUBLIC RAFT_HAS_GRPC=1)
endif()

# ── Test binary (requires Google Test) ────────────────────────
find_package(GTest QUIET)

if(GTest_FOUND)
    message(STATUS "Found Google Test — building raft_test")
    enable_testing()

    add_executable(raft_test
        test/test_main.cpp
        src/config.cpp
    )
    target_include_directories(raft_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    if(RAFT_HAS_GRPC)
        target_include_directories(raft_test PRIVATE ${PROTO_GEN_DIR})
    endif()
    # Prefer static GTest to avoid dyld rpath issues on macOS
    set(GTest_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
    target_link_libraries(raft_test PRIVATE raft_core GTest::gtest_main)

    # Set rpath for macOS dylib loading (Homebrew installs to /opt/homebrew/lib)
    if(APPLE)
        set_target_properties(raft_test PROPERTIES
            BUILD_RPATH "/opt/homebrew/lib"
            INSTALL_RPATH "/opt/homebrew/lib"
        )
    endif()

    # Use add_test instead of gtest_discover_tests to avoid running the
    # binary at configure/build time (which fails if dylibs aren't found yet).
    add_test(NAME RaftTests COMMAND raft_test)
else()
    message(STATUS "Google Test not found — skipping test target")
    message(STATUS "  Install with: brew install googletest  (macOS)")
    message(STATUS "            or: apt install libgtest-dev (Linux)")
endif()

# ── KV Raft (requires RocksDB + GTest) ───────────────────────
if(RAFT_HAS_ROCKSDB AND GTest_FOUND)
    message(STATUS "Building KV Raft test binary")

    add_library(kvraft_core STATIC
        src/kvstatemachine.cpp
        src/kvserver.cpp
        src/clerk.cpp
    )
    target_include_directories(kvraft_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(kvraft_core PUBLIC raft_core)

    # Link RocksDB
    if(RocksDB_FOUND)
        target_link_libraries(kvraft_core PUBLIC RocksDB::rocksdb)
    else()
        target_include_directories(kvraft_core PUBLIC ${ROCKSDB_INCLUDE_DIRS})
        target_link_libraries(kvraft_core PUBLIC ${ROCKSDB_LIBRARIES})
    endif()

    add_executable(kvraft_test
        test/kvtest_main.cpp
    )
    target_include_directories(kvraft_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(kvraft_test PRIVATE kvraft_core GTest::gtest_main)

    # Set rpath for macOS dylib loading
    if(APPLE)
        set_target_properties(kvraft_test PROPERTIES
            BUILD_RPATH "/opt/homebrew/lib"
            INSTALL_RPATH "/opt/homebrew/lib"
        )
    endif()

    add_test(NAME KVRaftTests COMMAND kvraft_test)
endif()

# ── ShardCtrler (requires GTest only, no RocksDB) ───────────
if(GTest_FOUND)
    message(STATUS "Building ShardCtrler test binary")

    add_library(shardctrler_core STATIC
        src/sc_statemachine.cpp
        src/shardctrler.cpp
        src/sc_clerk.cpp
    )
    target_include_directories(shardctrler_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(shardctrler_core PUBLIC raft_core)

    add_executable(shardctrler_test
        test/sctest_main.cpp
    )
    target_include_directories(shardctrler_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(shardctrler_test PRIVATE shardctrler_core GTest::gtest_main)

    add_test(NAME ShardCtrlerTests COMMAND shardctrler_test)
endif()

# ── ShardKV (requires GTest only, no RocksDB) ────────────────
if(GTest_FOUND)
    message(STATUS "Building ShardKV test binary")

    add_library(shardkv_core STATIC
        src/shardkv.cpp
        src/skv_clerk.cpp
    )
    target_include_directories(shardkv_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(shardkv_core PUBLIC raft_core shardctrler_core)

    add_executable(shardkv_test
        test/skvtest_main.cpp
    )
    target_include_directories(shardkv_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(shardkv_test PRIVATE shardkv_core GTest::gtest_main)

    add_test(NAME ShardKVTests COMMAND shardkv_test)
endif()
