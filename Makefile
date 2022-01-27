default: image build-test

BUILDER_IMAGE := $(or $(BUILDER_IMAGE),hub.docker.com/eraftio/eraft_redis)

image:
	docker build -f Dockerfile -t $(BUILDER_IMAGE) .

build-dev:
	chmod +x build.sh
	docker run --rm -v ${PWD}:/eraft hub.docker.com/eraftio/eraft_redis:latest /eraft/build.sh;

test:
	docker run --rm -v ${PWD}:/eraft hub.docker.com/eraftio/eraft_redis:latest /eraft/build/raftcore/test/raft_tests;
