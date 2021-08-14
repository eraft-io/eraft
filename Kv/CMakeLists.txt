# MIT License

# Copyright (c) 2021 Colin

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
    include/Kv/concurrency_queue.h
    include/Kv/peer.h
    include/Kv/raft_client.h
    include/Kv/raft_server.h
    include/Kv/raft_store.h
    include/Kv/raft_worker.h
    include/Kv/router.h
    include/Kv/server.h
    include/Kv/server_transport.h
    include/Kv/storage.h
    include/Kv/store_worker.h
    include/Kv/transport.h
    include/Kv/utils.h
)

set(Sources
    server/bootstrap.cc
    server/engines.cc
    server/file.cc
    server/node.cc
    server/peer_msg_handler.cc
    server/peer_storage.cc
    server/concurrency_queue.cc
    server/peer.cc
    server/raft_client.cc
    server/raft_server.cc
    server/raft_store.cc
    server/raft_worker.cc
    server/router.cc
    server/server.cc
    server/store_worker.cc
    server/server_transport.cc
    server/ticker.cc
    server/utils.cc
)

add_library(${This} STATIC ${Sources} ${Headers})
set_target_properties(${This} PROPERTIES
    FOLDER Libraries
)

target_include_directories(${This} PUBLIC include)

target_link_libraries(${This} PUBLIC
    gtest
    Logger
    rocksdb
    gRPC::grpc++
    Protocol
    RaftCore
    ${GFLAGS_LIBRARIES}
)

add_subdirectory(kvserver)
add_subdirectory(kvcli)
