#!/bin/bash

set -xe

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
SRCPATH=$(cd $SCRIPTPATH/..; pwd -P)
NPROC=$(nproc || grep -c ^processor /proc/cpuinfo)

if [ -d "$SRCPATH/Protocol" ]; then
  cd "$SRCPATH/Protocol"
  chmod -R 755 scripts
  ./scripts/generate_cpp.sh
  cd -
fi

build_dir="$SRCPATH/build_"
mkdir -p $build_dir && cd $build_dir
cmake "$SRCPATH" \
    -DENABLE_TESTS=on
make -j `nproc`

if [ ! -d "$SRCPATH/output" ]; then
  mkdir $SRCPATH/output
  mkdir $SRCPATH/output/logs
fi

cp $build_dir/RaftCore/test/RaftTests $SRCPATH/output
cp $build_dir/Logger/test/logger_tests $SRCPATH/output

$build_dir/RaftCore/test/RaftTests
$build_dir/Logger/test/logger_tests
