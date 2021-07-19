default: image build-test

# TODO: to dockerhub create eraftio account and eraft repository
BUILDER_IMAGE := $(or $(BUILDER_IMAGE),hub.docker.com/eraftio/eraft)

image:
	docker build -f Dockerfile -t $(BUILDER_IMAGE) .

build-dev:
	chmod +x build-test.sh
	# This images is pull from my(LiuJ) docker hub, you can build with the Dockerfile by youself.
	docker run --rm -v $(realpath ..):/eraft cxspace/ti-redis-dev:v0.0.1 /eraft/ci/build-test.sh;

run:
	docker run -it --rm -v $(realpath ..):/eraft cxspace/ti-redis-dev:v0.0.1 /eraft/ci/run-server.sh

# clean:
# 	docker rm $(docker ps -aq)
