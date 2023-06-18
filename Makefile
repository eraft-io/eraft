# MIT License

# Copyright (c) 2023 ERaftGroup

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
default: image

IMAGE_VERSION := v0.0.6

BUILDER_IMAGE := $(or $(BUILDER_IMAGE),eraft/eraftkv:$(IMAGE_VERSION))

# build base image from Dockerfile
image:
	docker build -f Dockerfile --network=host -t $(BUILDER_IMAGE) .

# generate protobuf cpp source file
gen-protocol-code:
	docker run --rm -v $(realpath .):/eraft eraft/eraftkv:v0.0.5  /usr/local/bin/protoc --grpc_out /eraft/src/ --plugin=protoc-gen-grpc=/grpc/.build/grpc_cpp_plugin -I /eraft/protocol /eraft/protocol/eraftkv.proto
	docker run --rm -v $(realpath .):/eraft eraft/eraftkv:v0.0.5  /usr/local/bin/protoc --cpp_out /eraft/src/ -I /eraft/protocol /eraft/protocol/eraftkv.proto

# build eraftkv on local machine
build-dev:
	chmod +x utils/build-dev.sh
	docker run -it --rm -v  $(realpath .):/eraft eraft/eraftkv:v0.0.6 /eraft/utils/build-dev.sh

# run all unit test
tests:
	chmod +x utils/run-tests.sh
	docker run -it --rm -v  $(realpath .):/eraft eraft/eraftkv:v0.0.6 /eraft/utils/run-tests.sh

create-net:
	docker network create --subnet=172.18.0.0/16 mytestnetwork

rm-net:
	docker network rm mytestnetwork

run-demo:
	docker run --name kvserver-node1 --network mytestnetwork --ip 172.18.0.2 -d --rm -v $(realpath .):/eraft eraft/eraftkv:v0.0.6 /eraft/build/eraftkv 0 /tmp/kv_db0 /tmp/log_db0 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090
	sleep 4s
	docker run --name kvserver-node2 --network mytestnetwork --ip 172.18.0.3 -d --rm -v $(realpath .):/eraft eraft/eraftkv:v0.0.6 /eraft/build/eraftkv 1 /tmp/kv_db1 /tmp/log_db1 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090
	docker run --name kvserver-node3 --network mytestnetwork --ip 172.18.0.4 -d --rm -v $(realpath .):/eraft eraft/eraftkv:v0.0.6 /eraft/build/eraftkv 2 /tmp/kv_db2 /tmp/log_db2 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090
	sleep 10s
	docker run --name vdbserver-node --network mytestnetwork --ip 172.18.0.6 -d --rm -v $(realpath .):/eraft eraft/eraftkv:v0.0.6 /eraft/build/eraft-vdb 172.18.0.6:12306 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090

stop-demo:
	docker stop kvserver-node1 kvserver-node2 kvserver-node3 vdbserver-node

run-demo-bench:
	docker run --name kvserver-bench --network mytestnetwork --ip 172.18.0.5 --rm -v $(realpath .):/eraft eraft/eraftkv:v0.0.6 /eraft/build/eraftkv-ctl 172.18.0.2:8088 bench 64 64 10

run-vdb-tests:
	chmod +x utils/run-vdb-tests.sh
	docker run --name vdbserver-node-tests --network mytestnetwork --ip 172.18.0.8 -it --rm -v $(realpath .):/eraft eraft/eraftkv:v0.0.6 /eraft/utils/run-vdb-tests.sh
