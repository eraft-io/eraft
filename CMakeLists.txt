# CMakeLists.txt for ectdraftcpp
# 
# © 2020 by LiuJ

cmake_minimum_required(VERSION 3.8)
set(This ectdraftcpp)
project(${This} C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

enable_testing()

get_directory_property(ParentDirectory PARENT_DIRECTORY)
if(ParentDirectory STREQUAL "")
    # Define option for linking with runtime statically
    if(MSVC)
        option(RUNTIME_STATIC_LINKAGE "Link with runtime library statically" OFF)
    endif(MSVC)

    if(MSVC AND RUNTIME_STATIC_LINKAGE)
        foreach(flag
            CMAKE_C_FLAGS
            CMAKE_CXX_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL
            CMAKE_CXX_FLAGS_MINSIZEREL
            CMAKE_C_FLAGS_RELWITHDEBINFO
            CMAKE_CXX_FLAGS_RELWITHDEBINFO
        )
            if(${flag} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MI" ${flag} "${${flag}}")
            endif(${flag} MATCHES "/MD")
        endforeach(flag)
    endif(MSVC AND RUNTIME_STATIC_LINKAGE)

    # Keep Google Test's linkage of the runtime in sync our own
    if(RUNTIME_STATIC_LINKAGE)
        set(gtest_force_shared_crt OFF CACHE BOOL "Use shared (DLL) run-time lib even when Google Test is built as static lib." FORCE)
    else(RUNTIME_STATIC_LINKAGE)
        set(gtest_force_shared_crt ON CACHE BOOL "Use shared (DLL) run-time lib even when Google Test is built as static lib." FORCE)
    endif(RUNTIME_STATIC_LINKAGE)

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
    set_target_properties(gtest gtest_main PROPERTIES
        FOLDER libraries
    )

    # grpc
    include_directories(grpc/include)

    # gRPC and relateds

    find_package (OpenSSL)
    message (STATUS "Using ssl=${OPENSSL_FOUND}: ${OPENSSL_INCLUDE_DIR} : ${OPENSSL_LIBRARIES}")

    find_package(Protobuf REQUIRED)
    message(STATUS "Using protobuf: ${Protobuf_VERSION} : ${Protobuf_INCLUDE_DIRS}, ${Protobuf_LIBRARIES}")

    include_directories(${PROTOBUF_INCLUDE_DIRS})

    find_package(c-ares REQUIRED)
    message(STATUS "Lib c-ares found")

    find_package(ZLIB REQUIRED)
    message(STATUS "Using ZLIB: ${ZLIB_INCLUDE_DIRS}, ${ZLIB_LIBRARIES}")

    find_package(gRPC CONFIG REQUIRED)
    message(STATUS "Using gRPC: ${gRPC_VERSION}")

    # add_subdirectory(leveldb)
    add_subdirectory(Logger)
    add_subdirectory(Protocol)
    add_subdirectory(RaftCore)
    add_subdirectory(Kv)
    add_subdirectory(protobuf)
    add_subdirectory(rocksdb)

endif(ParentDirectory STREQUAL "")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
