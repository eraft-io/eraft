default: image build-test

BUILDER_IMAGE := $(or $(BUILDER_IMAGE),hub.docker.com/eraftio/eraft)

image:
	docker build -f Dockerfile -t $(BUILDER_IMAGE) .

build-dev:
	chmod +x build-dev.sh
	# This images is pull from my(LiuJ) docker hub, you can build with the Dockerfile by youself.
	docker run --rm -v $(realpath ..):/eraft hub.docker.com/eraftio/eraft:latest /eraft/ci/build-dev.sh;

clean:
	docker rm $(docker ps -aq)
