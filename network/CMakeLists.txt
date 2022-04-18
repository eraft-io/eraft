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
set(This network)

set(Headers 
    include/network/async_buffer.h
    include/network/buffer.h
    include/network/client_socket.h
    include/network/client.h
    include/network/command.h
    include/network/common.h
    include/network/concurrency_msg_queue.h
    include/network/config.h
    include/network/epoller.h
    include/network/executor.h
    include/network/file_sys.h
    include/network/listen_socket.h
    include/network/lock_free_queue.h
    include/network/net_thread_pool.h
    include/network/poller.h
    include/network/proto_parser.h
    include/network/raft_client.h
    include/network/raft_config.h
    include/network/raft_node.h
    include/network/raft_peer_msg_handler.h
    include/network/raft_peer_storage.h
    include/network/raft_peer.h
    include/network/raft_router.h
    include/network/raft_server_transport.h
    include/network/raft_server.h
    include/network/raft_storage_interface.h
    include/network/raft_store.h
    include/network/raft_ticker.h
    include/network/raft_utils.h
    include/network/raft_worker.h
    include/network/regex.h
    include/network/server.h
    include/network/socket.h
    include/network/task_manager.h
    include/network/thread_pool.h
    include/network/toml.h
    include/network/transport_interface.h
    include/network/unbounded_buffer.h
)

set(Sources
    src/async_buffer.cc
    src/client.cc
    src/cmd_string.cc
    src/cmd.cc
    src/common.cc
    src/config.cc
    src/epoller.cc
    src/executor.cc
    src/listen_socket.cc
    src/net_thread_pool.cc
    src/proto_parser.cc
    src/server.cc
    src/socket.cc
    src/stream_socket.cc
    src/task_manager.cc
    src/thread_pool.cc
    src/unbounded_buffer.cc
    src/regex.cc
)

add_library(${This} STATIC ${Sources} ${Headers})
set_target_properties(${This} PROPERTIES
    FOLDER Libraries
)

target_include_directories(${This} PUBLIC include)

target_link_libraries(${This} PUBLIC
    protocol
    gtest_main
    pmemkv
    pthread
)
