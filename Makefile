default: image build-test

BUILDER_IMAGE := $(or $(BUILDER_IMAGE),hub.docker.com/eraftio/eraft_pmem)

# we build image and push to dockerhub already，if 
# you want to make it yourself，don't forget to change 
# url at make build-dev and make test
image:
	docker build -f Dockerfile -t $(BUILDER_IMAGE) .

build-dev:
	chmod +x build.sh
	docker run --rm -v ${PWD}:/eraft eraft/eraft_pmem:v2 /eraft/build.sh;

run-single:
	docker run --rm -it -p 127.0.0.1:6379:6379 --user root -v ${PWD}:/eraft eraft/eraft_pmem:v2 /eraft/scripts/run-single.sh

into:
	docker run --rm -it -p 127.0.0.1:6379:6379 --user root -v ${PWD}:/eraft eraft/eraft_pmem:v2 /bin/bash

test:
	docker run --rm -v ${PWD}:/eraft eraft/eraft_pmem:v2 /eraft/build_/raftcore/test/raft_tests;
