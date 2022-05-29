#!/bin/bash
. ~/.bash_profile
export GO111MODULE="off"
go build -txn -o /root/go/src/github.com/eraft-io/eraft/output/eng_bench /root/go/src/github.com/eraft-io/eraft/cmd/eng_bench/eng_bench.go
