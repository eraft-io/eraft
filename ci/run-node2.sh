#!/bin/bash

set -xe

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
SRCPATH=$(cd $SCRIPTPATH/..; pwd -P)

build_dir="$SRCPATH/build_"
$build_dir/Kv/kvserver/kv_svr 127.0.0.1:20161 /tmp/db2 2

# ./kv_svr 127.0.0.1:20161 /tmp/db2 2