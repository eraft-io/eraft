#!/usr/bin/env bash

SCRIPTS_DIR=$(dirname "$0")
source $SCRIPTS_DIR/common.sh

echo "generate cpp code..."

KVPROTO_ROOT="$SCRIPTS_DIR/.."

cd $KVPROTO_ROOT
rm -rf proto-cpp && mkdir -p proto-cpp
rm -rf eraftio && mkdir eraftio

cp proto/* proto-cpp/

sed_inplace '/gogo.proto/d' proto-cpp/*
sed_inplace '/option\ *(gogoproto/d' proto-cpp/*
sed_inplace -e 's/\[.*gogoproto.*\]//g' proto-cpp/*

push proto-cpp
protoc --cpp_out ../eraftio/ *.proto || exit $?
pop

rm -rf proto-cpp
