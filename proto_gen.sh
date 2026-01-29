#!/bin/sh
export PATH="$PATH:$(go env GOPATH)/bin"

go install google.golang.org/protobuf/cmd/protoc-gen-go@v1.28
go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@v1.2

protoc -I=/usr/local/include -I ./raftpb ./raftpb/raft.proto --go_out=./raftpb --go-grpc_out=./raftpb