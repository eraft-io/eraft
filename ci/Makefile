default: image build-test

# TODO: to dockerhub create eraftio account and eraft repository
BUILDER_IMAGE := $(or $(BUILDER_IMAGE),hub.docker.com/eraftio/eraft)

image:
	docker build -f Dockerfile -t $(BUILDER_IMAGE) .

build-dev:
	chmod +x build-test.sh
	# This images is pull from my(LiuJ) docker hub, you can build with the Dockerfile by youself.
	docker run --rm -v $(realpath ..):/eraft cxspace/ti-redis-dev:v0.0.1 /eraft/ci/build-test.sh;

#
# TODO: cluster docker version
# run1:
# 	docker run -it --rm -v $(realpath ..):/eraft -p 20160:20160 cxspace/ti-redis-dev:v0.0.1 /eraft/ci/run-node1.sh

# run2:
# 	docker run -it --rm -v $(realpath ..):/eraft -p 20161:20161 cxspace/ti-redis-dev:v0.0.1 /eraft/ci/run-node2.sh

# run3:
# 	docker run -it --rm -v $(realpath ..):/eraft -p 20162:20162 cxspace/ti-redis-dev:v0.0.1 /eraft/ci/run-node3.sh

# clean:
# 	docker rm $(docker ps -aq)
