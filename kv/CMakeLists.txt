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
set(This kv)

set(Headers 
    include/kv/bench_tools.h
    include/kv/bootstrap.h
    include/kv/callback.h
    include/kv/config.h
    include/kv/engines.h
    include/kv/file.h
    include/kv/msg.h
    include/kv/node.h
    include/kv/peer_msg_handler.h
    include/kv/peer_storage.h
    include/kv/concurrentqueue.h
    include/kv/concurrency_queue.h
    include/kv/peer.h
    include/kv/raft_client.h
    include/kv/raft_server.h
    include/kv/raft_store.h
    include/kv/raft_worker.h
    include/kv/rate_limiter.h
    include/kv/router.h
    include/kv/server.h
    include/kv/server_transport.h
    include/kv/storage.h
    include/kv/store_worker.h
    include/kv/transport.h
    include/kv/utils.h
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
    server/rate_limiter.cc
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
    rocksdb
    snappy
    bz2
    zstd
    lz4
    z
    gRPC::grpc++
    protocol
    raftcore
    ${GFLAGS_LIBRARIES}
)

add_subdirectory(kvserver)
add_subdirectory(kvcli)
add_subdirectory(kvbench)
