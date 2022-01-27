FROM ubuntu:18.04

RUN apt update -y && \
    apt install -y cmake protobuf-compiler git gcc g++

RUN git clone --branch v1.9.2 https://github.com/gabime/spdlog.git && cd spdlog && mkdir build && cd build \
       && cmake .. && make -j && make install

RUN apt install -y libprotobuf-dev
