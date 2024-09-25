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

IMAGE_VERSION := v0.0.1

BUILDER_IMAGE := $(or $(BUILDER_IMAGE),eraft/eraftbook:$(IMAGE_VERSION))

default: meta_cli shard_server shard_cli meta_server kv_server kv_cli

image:
	docker build -f Dockerfile --network=host -t $(BUILDER_IMAGE) .

build-dev:
	chmod +x scripts/build_dev.sh
	docker run -it --rm -v  $(realpath .):/eraft eraft/eraftbook:$(IMAGE_VERSION) /eraft/scripts/build_dev.sh

run-test:
	chmod +x scripts/run_tests.sh
	docker run --name test-cli-node --network mytestnetwork --ip 172.18.0.5 -it --rm -v  $(realpath .):/eraft eraft/eraftbook:$(IMAGE_VERSION) /eraft/scripts/run_tests.sh

meta_cli:
	go build -o output/metacli cmd/metacli/metacli.go

meta_server:
	go build -o output/metaserver cmd/metasvr/metasvr.go

shard_server:
	go build -o output/shardserver cmd/shardsvr/shardsvr.go

shard_cli:
	go build -o output/shardcli cmd/shardcli/shardcli.go

kv_server:
	go build -o output/kvserver cmd/kvraft/kvserver.go

kv_cli:
	go build -o output/kvcli cmd/kvcli/kvcli.go

clean:
	rm -rf output/*

create-net:
	docker network create --subnet=172.18.0.0/16 mytestnetwork

run-demo:
	docker run --name metaserver-node1 --network mytestnetwork --ip 172.18.0.2 --privileged=true -d --rm -v $(realpath .):/eraft eraft/eraftbook:$(IMAGE_VERSION) /eraft/output/metaserver 0 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090
	docker run --name metaserver-node2 --network mytestnetwork --ip 172.18.0.3 --privileged=true -d --rm -v $(realpath .):/eraft eraft/eraftbook:$(IMAGE_VERSION) /eraft/output/metaserver 1 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090
	docker run --name metaserver-node3 --network mytestnetwork --ip 172.18.0.4 --privileged=true -d --rm -v $(realpath .):/eraft eraft/eraftbook:$(IMAGE_VERSION) /eraft/output/metaserver 2 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090
	sleep 1
	docker run --name kvserver-node1 --network mytestnetwork --ip 172.18.0.10 --privileged=true -d --rm -v $(realpath .):/eraft eraft/eraftbook:$(IMAGE_VERSION) /eraft/output/shardserver 0 1 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090 172.18.0.10:8088,172.18.0.11:8089,172.18.0.12:8090 
	docker run --name kvserver-node2 --network mytestnetwork --ip 172.18.0.11 --privileged=true -d --rm -v $(realpath .):/eraft eraft/eraftbook:$(IMAGE_VERSION) /eraft/output/shardserver 1 1 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090 172.18.0.10:8088,172.18.0.11:8089,172.18.0.12:8090 
	docker run --name kvserver-node3 --network mytestnetwork --ip 172.18.0.12 --privileged=true -d --rm -v $(realpath .):/eraft eraft/eraftbook:$(IMAGE_VERSION) /eraft/output/shardserver 2 1 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090 172.18.0.10:8088,172.18.0.11:8089,172.18.0.12:8090 
	docker run --name kvserver-node4 --network mytestnetwork --ip 172.18.0.13 --privileged=true -d --rm -v $(realpath .):/eraft eraft/eraftbook:$(IMAGE_VERSION) /eraft/output/shardserver 0 2 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090 172.18.0.13:8088,172.18.0.14:8089,172.18.0.15:8090 
	docker run --name kvserver-node5 --network mytestnetwork --ip 172.18.0.14 --privileged=true -d --rm -v $(realpath .):/eraft eraft/eraftbook:$(IMAGE_VERSION) /eraft/output/shardserver 1 2 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090 172.18.0.13:8088,172.18.0.14:8089,172.18.0.15:8090 
	docker run --name kvserver-node6 --network mytestnetwork --ip 172.18.0.15 --privileged=true -d --rm -v $(realpath .):/eraft eraft/eraftbook:$(IMAGE_VERSION) /eraft/output/shardserver 2 2 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090 172.18.0.13:8088,172.18.0.14:8089,172.18.0.15:8090 

stop-demo:
	docker stop kvserver-node1 kvserver-node2 kvserver-node3 kvserver-node4 kvserver-node5 kvserver-node6 metaserver-node1 metaserver-node2 metaserver-node3


