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

FROM eraft/eraftkv:v0.0.6

# ENV TZ=Asia/Kolkata \
#     DEBIAN_FRONTEND=noninteractive

# # install build dependency
# RUN apt-get update && apt-get install -y clang-format build-essential autoconf automake libtool lcov libgflags-dev libsnappy-dev zlib1g-dev libbz2-dev libzstd-dev git wget

# # install latest cmake
# RUN wget https://github.com/Kitware/CMake/releases/download/v3.26.3/cmake-3.26.3-linux-x86_64.sh -q -O /tmp/cmake-install.sh && chmod u+x /tmp/cmake-install.sh \
#  && mkdir /opt/cmake-3.26.3 && /tmp/cmake-install.sh --skip-license --prefix=/opt/cmake-3.26.3 \
#   && rm /tmp/cmake-install.sh && ln -s /opt/cmake-3.26.3/bin/* /usr/local/bin

# # install rocksdb
# RUN apt-get update && apt-get install -y librocksdb-dev libssl-dev

# # install gtest
# RUN apt-get install -y libgtest-dev && cd /usr/src/gtest && cmake CMakeLists.txt && make && cp lib/*.a /usr/lib && ln -s /usr/lib/libgtest.a /usr/local/lib/libgtest.a && ln -s /usr/lib/libgtest_main.a /usr/local/lib/libgtest_main.a

# RUN apt update -y \
#        && apt install -y ccache libssl-dev libcrypto++-dev \
#        libglib2.0-dev \
#        gcc-10 g++-10 \
#        && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 20 \
#        && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 20

# RUN git clone https://github.com/grpc/grpc.git && cd grpc && git checkout v1.28.0 && git submodule update --init && \
# mkdir .build && cd .build && cmake .. -DgRPC_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release \
# && make install -j4 && cd .. && rm -rf .build/CMakeCache.txt && cd .build && 
# cmake .. -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DgRPC_PROTOBUF_PROVIDER=package -DgRPC_ZLIB_PROVIDER=package -DgRPC_CARES_PROVIDER=package -DgRPC_SSL_PROVIDER=package -DCMAKE_BUILD_TYPE=Release && make install -j4

# intstall google benchmark
# RUN git clone https://github.com/google/benchmark.git && git clone https://github.com/google/googletest.git benchmark/googletest && cd benchmark && cmake -E make_directory "build" && cmake -E chdir "build" cmake -DCMAKE_BUILD_TYPE=Release ../ && cmake --build "build" --config Release --target install

# RUN cd /grpc/third_party/protobuf && ./autogen.sh && ./configure && make -j8 && make install

RUN git clone https://github.com/jupp0r/prometheus-cpp.git && cd prometheus-cpp && git submodule init && git submodule update && mkdir _build && cd _build && cmake .. -DBUILD_SHARED_LIBS=ON -DENABLE_PUSH=OFF -DENABLE_COMPRESSION=OFF && cmake --build . --parallel 4 && cmake --install .

RUN apt-get install curl -y
