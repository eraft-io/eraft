# CMakeLists.txt for Logger
# 
# © 2020 by LiuJ

cmake_minimum_required(VERSION 3.8)
set(This Kv)

set(Headers 
    include/Kv/bootstrap.h
    include/Kv/callback.h
    include/Kv/config.h
    include/Kv/engines.h
    include/Kv/file.h
    include/Kv/msg.h
    include/Kv/node.h
    include/Kv/peer_msg_handler.h
    include/Kv/peer_storage.h
    include/Kv/peer.h
    include/Kv/raft_client.h
    include/Kv/raft_server.h
    include/Kv/raft_store.h
    include/Kv/raft_worker.h
    include/Kv/router.h
    include/Kv/server_impl.h
    include/Kv/server_transport.h
    include/Kv/storage.h
    include/Kv/store_worker.h
    include/Kv/transport.h
    include/Kv/util.h
    include/Kv/write_batch.h
)

set(Sources
    server/bootstrap.cc
    server/engines.cc
    server/file.cc
    server/node.cc
    server/peer_msg_handler.cc
    server/peer_storage.cc
    server/peer.cc
    server/raft_client.cc
    server/raft_server.cc
    server/raft_store.cc
    server/raft_worker.cc
    server/router.cc
    server/server_impl.cc
    server/store_worker.cc
    server/server_transport.cc
)

add_library(${This} STATIC ${Sources} ${Headers})
set_target_properties(${This} PROPERTIES
    FOLDER Libraries
)

target_include_directories(${This} PUBLIC include)

target_link_libraries(${This} PUBLIC
    gtest_main
    Logger
    leveldb
    gRPC::grpc++
    Protocol
    RaftCore
)

add_subdirectory(kvserver)

add_subdirectory(test_cli)
add_subdirectory(test_svr)
add_subdirectory(test_kvcli)
