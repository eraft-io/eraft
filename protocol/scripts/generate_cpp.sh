#!/usr/bin/env bash

SCRIPTS_DIR=$(dirname "$0")
source $SCRIPTS_DIR/common.sh

echo "generate cpp code..."

KVPROTO_ROOT="$SCRIPTS_DIR/.."
GRPC_INCLUDE=.:../include
GRPC_CPP_PLUGIN=$(which grpc_cpp_plugin)

cd $KVPROTO_ROOT
rm -rf proto-cpp && mkdir -p proto-cpp
rm -rf eraftio && mkdir eraftio

cp proto/* proto-cpp/

sed_inplace '/gogo.proto/d' proto-cpp/*
sed_inplace '/option\ *(gogoproto/d' proto-cpp/*
sed_inplace -e 's/\[.*gogoproto.*\]//g' proto-cpp/*

push proto-cpp
protoc -I${GRPC_INCLUDE} --cpp_out ../eraftio/ *.proto || exit $?
pop

rm -rf proto-cpp
