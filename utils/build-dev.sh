#!/bin/bash
set -xe

# if use an old docker image
apt-get install libgtest-dev -y && cd /usr/src/gtest && cmake CMakeLists.txt && make && mv lib/* /usr/lib

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
SRCPATH=$(cd $SCRIPTPATH/..; pwd -P)
NPROC=$(nproc || grep -c ^processor /proc/cpuinfo)

BUILD_DIR="$SRCPATH/build"
mkdir -p $BUILD_DIR && cd $BUILD_DIR
cmake "$SRCPATH" \
    -DENABLE_TESTS=ON \
    -DDOWNLOAD_GRPC_CN=ON
make -j $NPROC

# run test exe
/eraft/build/gtest_example_tests
