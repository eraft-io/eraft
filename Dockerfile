# MIT License

# Copyright (c) 2023 ERaftGroup

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

FROM ubuntu:20.04

ENV TZ=Asia/Kolkata \
    DEBIAN_FRONTEND=noninteractive

# install build dependency
RUN apt-get update && apt-get install -y clang-format build-essential autoconf automake libtool cmake lcov libgflags-dev libsnappy-dev zlib1g-dev libbz2-dev libzstd-dev git wget

# install rocksdb
RUN apt-get update && apt-get install -y librocksdb-dev

# install protoc
RUN apt install -y protobuf-compiler

# build grpc_cpp_plugin 
RUN git clone -b feature_20230323_initdesign https://github.com/eraft-io/eraft.git && cd eraft && mkdir build && cd build && cmake .. && make -j4 && mv /eraft/build/_deps/grpc-build/grpc_cpp_plugin /usr/bin/ && rm -rf /eraft

# install gtest
RUN apt-get install libgtest-dev -y && cd /usr/src/gtest && cmake CMakeLists.txt && make && mv lib/* /usr/lib
