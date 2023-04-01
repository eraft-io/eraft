# Makefile
default: image

BUILDER_IMAGE := $(or $(BUILDER_IMAGE),hub.docker.com/eraftio/eraftkv)

# build base image from Dockerfile
image:
	docker build -f Dockerfile -t $(BUILDER_IMAGE) .

# generate protobuf cpp source file
gen-protocol-code:
	docker run --rm -v ${PWD}:/eraft-outer hub.docker.com/eraftio/eraftkv:latest protoc --grpc_out /eraft-outer/src/ --plugin=protoc-gen-grpc=/usr/bin/grpc_cpp_plugin -I /eraft-outer/protocol eraftkv.proto
	docker run --rm -v ${PWD}:/eraft-outer hub.docker.com/eraftio/eraftkv:latest protoc --cpp_out /eraft-outer/src/ -I /eraft-outer/protocol eraftkv.proto

build-dev:
	echo "TO SUPPORT"

tests:
	echo "TO SUPPORT"
