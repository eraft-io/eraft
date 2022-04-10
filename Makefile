default: image build-test

BUILDER_IMAGE := $(or $(BUILDER_IMAGE),hub.docker.com/eraftio/eraft_pmem)

# we build image and push to dockerhub already，if 
# you want to make it yourself，don't forget to change 
# url at make build-dev and make test
image:
	docker build -f Dockerfile -t $(BUILDER_IMAGE) .

build-dev:
	chmod +x build.sh
	docker run --rm -v ${PWD}:/eraft eraft/eraft_pmem:v1 /eraft/build.sh;

test:
	docker run --rm -v ${PWD}:/eraft eraft/eraft_pmem:v1 /eraft/build_/raftcore/test/raft_tests;
