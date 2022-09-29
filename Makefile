# MIT License

# Copyright (c) 2022 eraft dev group

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

default: cfgcli cfgserver shardserver shardcli bench_cli eng_bench kv_server kvcli rdserver

kvcli:
	go build -o output/kvcli cmd/kvcli/kvcli.go

kv_server:
	go build -o output/kv_server cmd/kvraft/kvserver.go

pmemserver:
	go build -txn -o output/pmemserver cmd/pmemsvr/pmemsvr.go

cfgcli:
	go build -o output/cfgcli cmd/configcli/cfg_cli.go

cfgserver:
	go build -o output/cfgserver cmd/configsvr/cfg_svr.go

rdserver:
	go build -o output/rds_server cmd/rds_server/rds_svr.go

shardserver:
	go build -o output/shardserver cmd/shardsvr/shard_svr.go

shardcli:
	go build -o output/shardcli cmd/shardcli/shard_cli.go

bench_cli:
	go build -o output/bench_cli cmd/benchcli/bench_cli.go

eng_bench:
	go build -o output/eng_bench cmd/eng_bench/eng_bench.go

eng_pmem:
	go build -o output/eng_bench cmd/eng_bench/eng_bench.go

into_pmem0:
	docker run --rm -it -p 0.0.0.0:6379:6379 -v ${PWD}:/eraft eraft/eraft_pmem_redis:v1 /bin/bash

into_pmem1:
	docker run --rm -it -p 0.0.0.0:6380:6380 -v ${PWD}:/eraft eraft/eraft_pmem_redis:v1 /bin/bash

into_pmem2:
	docker run --rm -it -p 0.0.0.0:6381:6381 -v ${PWD}:/eraft eraft/eraft_pmem_redis:v1 /bin/bash

build_pmem_bench:
	chmod +x build.sh; docker run --rm -p 6379:0.0.0.0:6379 -v ${PWD}:/root/go/src/github.com/eraft-io/eraft eraft/go_pmem_dev:v2 /root/go/src/github.com/eraft-io/eraft/build.sh

clean:
	rm -rf output/*
