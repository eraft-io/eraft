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

IMAGE_VERSION := v0.0.3

BUILDER_IMAGE := $(or $(BUILDER_IMAGE),eraft/eraftkv:$(IMAGE_VERSION))

# build base image from Dockerfile
image:
	docker build -f Dockerfile --network=host -t $(BUILDER_IMAGE) .

# generate protobuf cpp source file
gen-protocol-code:
	docker run --rm -v $(realpath .):/eraft eraft/eraftkv:v0.0.3 protoc --grpc_out /eraft/src/ --plugin=protoc-gen-grpc=/usr/bin/grpc_cpp_plugin -I /eraft/protocol eraftkv.proto
	docker run --rm -v $(realpath .):/eraft eraft/eraftkv:v0.0.3 protoc --cpp_out /eraft/src/ -I /eraft/protocol eraftkv.proto

# build eraftkv on local machine
build-dev:
	chmod +x utils/build-dev.sh
	docker run -it --rm -v  $(realpath .):/eraft eraft/eraftkv:v0.0.3 /eraft/utils/build-dev.sh

tests:
	docker run -it --rm -v  $(realpath .):/eraft eraft/eraftkv:v0.0.3 /eraft/build/gtest_example_tests
	docker run -it --rm -v  $(realpath .):/eraft eraft/eraftkv:v0.0.3 /eraft/build/rocksdb_storage_impl_tests
