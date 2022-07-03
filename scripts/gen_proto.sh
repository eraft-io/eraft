#!/bin/sh
export PATH="$PATH:$(go env GOPATH)/bin"

go install google.golang.org/protobuf/cmd/protoc-gen-go@v1.28
go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@v1.2

protoc -I ../pkg/protocol ../pkg/protocol/raftbasic.proto --go_out=../pkg/protocol --go-grpc_out=../pkg/protocol
protoc -I ../pkg/protocol ../pkg/protocol/wellwood.proto --go_out=../pkg/protocol --go-grpc_out=../pkg/protocol
