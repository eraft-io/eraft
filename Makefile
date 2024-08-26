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
	docker run --rm -v $(realpath .):/eraft eraft/eraftkv:v0.0.5  /usr/local/bin/protoc --grpc_out /eraft/raftcore/src/ --plugin=protoc-gen-grpc=/grpc/.build/grpc_cpp_plugin -I /eraft/protocol /eraft/protocol/eraftkv.proto
	docker run --rm -v $(realpath .):/eraft eraft/eraftkv:v0.0.5  /usr/local/bin/protoc --cpp_out /eraft/raftcore/src/ -I /eraft/protocol /eraft/protocol/eraftkv.proto

# build eraftkv on local machine
build-dev:
	chmod +x scripts/build-dev.sh
	docker run -it --rm -v  $(realpath .):/eraft eraft/eraftkv:$(IMAGE_VERSION) /eraft/scripts/build-dev.sh

create-net:
	docker network create --subnet=172.18.0.0/16 mytestnetwork

rm-net:
	docker network rm mytestnetwork

run-demo:
	@if [ ! -d "data" ]; then mkdir data; fi;
	@if [ ! -d "logs" ]; then mkdir logs; fi;
	docker run --name kvserver-node1 --network mytestnetwork -p 18080:18080 --ip 172.18.0.10 --privileged=true -d --rm -v $(realpath .):/eraft eraft/eraftkv:$(IMAGE_VERSION) /eraft/build/example/eraftkv -svr_id 0 -kv_db_path /eraft/data/kv_db0 -log_db_path /eraft/data/log_db0 -snap_db_path /eraft/data/snap_db0 -peer_addrs 172.18.0.10:8088,172.18.0.11:8089,172.18.0.12:8090 -log_file_path /eraft/logs/eraftkv-1.log -monitor_port 18080
	sleep 2
	docker run --name kvserver-node2 --network mytestnetwork -p 18081:18081 --ip 172.18.0.11 --privileged=true -d --rm -v $(realpath .):/eraft eraft/eraftkv:$(IMAGE_VERSION) /eraft/build/example/eraftkv -svr_id 1 -kv_db_path /eraft/data/kv_db1 -log_db_path /eraft/data/log_db1 -snap_db_path /eraft/data/snap_db1 -peer_addrs 172.18.0.10:8088,172.18.0.11:8089,172.18.0.12:8090 -log_file_path /eraft/logs/eraftkv-2.log -monitor_port 18081
	docker run --name kvserver-node3 --network mytestnetwork -p 18082:18082 --ip 172.18.0.12 --privileged=true -d --rm -v $(realpath .):/eraft eraft/eraftkv:$(IMAGE_VERSION) /eraft/build/example/eraftkv -svr_id 2 -kv_db_path /eraft/data/kv_db2 -log_db_path /eraft/data/log_db2 -snap_db_path /eraft/data/snap_db2 -peer_addrs 172.18.0.10:8088,172.18.0.11:8089,172.18.0.12:8090 -log_file_path /eraft/logs/eraftkv-3.log -monitor_port 18082
	sleep 1
	docker run --name metaserver-node1 --network mytestnetwork -p 19080:19080 --ip 172.18.0.2 -d --rm -v $(realpath .):/eraft eraft/eraftkv:$(IMAGE_VERSION) /eraft/build/example/eraftmeta -svr_id 0 -kv_db_path /eraft/data/meta_db0 -log_db_path /eraft/data/meta_log_db0 -peer_addrs 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090 -monitor_port 19080
	sleep 3
	docker run --name metaserver-node2 --network mytestnetwork -p 19081:19081 --ip 172.18.0.3 -d --rm -v $(realpath .):/eraft eraft/eraftkv:$(IMAGE_VERSION) /eraft/build/example/eraftmeta -svr_id 1 -kv_db_path /eraft/data/meta_db1 -log_db_path /eraft/data/meta_log_db1 -peer_addrs 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090 -monitor_port 19081
	docker run --name metaserver-node3 --network mytestnetwork -p 19082:19082 --ip 172.18.0.4 -d --rm -v $(realpath .):/eraft eraft/eraftkv:$(IMAGE_VERSION) /eraft/build/example/eraftmeta -svr_id 2 -kv_db_path /eraft/data/meta_db2 -log_db_path /eraft/data/meta_log_db2 -peer_addrs 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090 -monitor_port 19082

stop-demo:
	docker stop kvserver-node1 kvserver-node2 kvserver-node3 metaserver-node1 metaserver-node2 metaserver-node3

run-demo-test:
	docker run --name kvserver-bench --network mytestnetwork --ip 172.18.0.5 --rm -v $(realpath .):/eraft eraft/eraftkv:$(IMAGE_VERSION) /eraft/scripts/run-tests.sh
