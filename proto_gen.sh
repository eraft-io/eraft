#!/bin/sh
export PATH="$PATH:$(go env GOPATH)/bin"

go install google.golang.org/protobuf/cmd/protoc-gen-go@v1.28
go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@v1.2

protoc -I=/usr/local/include -I . ./raftpb/raft.proto --go_out=. --go_opt=paths=source_relative --go-grpc_out=. --go-grpc_opt=paths=source_relative
protoc -I=/usr/local/include -I . ./shardctrlerpb/shardctrler.proto --go_out=. --go_opt=paths=source_relative --go-grpc_out=. --go-grpc_opt=paths=source_relative
protoc -I=/usr/local/include -I . ./shardkvpb/shardkv.proto --go_out=. --go_opt=paths=source_relative --go-grpc_out=. --go-grpc_opt=paths=source_relative
protoc -I=/usr/local/include -I . ./kvraftpb/kvraft.proto --go_out=. --go_opt=paths=source_relative --go-grpc_out=. --go-grpc_opt=paths=source_relative