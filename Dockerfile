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
RUN apt-get update && apt-get install -y \
  build-essential autoconf libtool git pkg-config curl \
  automake libtool curl make g++ unzip cmake gdb \
  && apt-get clean

# go root home dir
RUN cd /root
# build grpc
RUN git clone -b v1.53.0 https://github.com/grpc/grpc && \
    cd grpc && \
    git submodule update --init && \
    cd ./third_party/protobuf && \
    ./autogen.sh && \
    ./configure --prefix=/opt/protobuf && \
    make -j `nproc` && make install && \
    cd ../.. && \
    make -j `nproc` PROTOC=/opt/protobuf/bin/protoc && \
    make prefix=/opt/grpc install

# build rocksdb
RUN git clone --branch v6.20.3 https://github.com/facebook/rocksdb.git && cd rocksdb && mkdir build && cd build \
       && cmake .. && make -j && make install && rm -rf build
