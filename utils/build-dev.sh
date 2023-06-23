#!/bin/bash
set -xe

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
SRCPATH=$(cd $SCRIPTPATH/..; pwd -P)
NPROC=$(nproc || grep -c ^processor /proc/cpuinfo)

BUILD_DIR="$SRCPATH/build"
mkdir -p $BUILD_DIR && cd $BUILD_DIR
cmake "$SRCPATH" \
    -DENABLE_TESTS=ON \
    -DDOWNLOAD_GRPC_CN=ON \
    -DFAISS_ENABLE_GPU=OFF \
    -DFAISS_ENABLE_PYTHON=OFF \
    -DBUILD_SHARED_LIBS=OFF

make -j $NPROC
