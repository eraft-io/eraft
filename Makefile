default: image

BUILDER_IMAGE := $(or $(BUILDER_IMAGE),hub.docker.com/eraftio/tinydb)

image:
	docker build -f Dockerfile -t $(BUILDER_IMAGE) .

build-dev:
	chmod +x build.sh
	docker run --user root --rm -v ${PWD}:/eraft hub.docker.com/eraftio/tinydb:latest /eraft/build.sh

run-dev:
	docker run -it -p 0.0.0.0:12306:12306 -v ${PWD}:/eraft hub.docker.com/eraftio/tinydb:latest /bin/bash

run:
	chmod +x run-test.sh
	docker run --rm --user root -p 0.0.0.0:12306:12306 -p  0.0.0.0:6379:6379 -it -v ${PWD}:/eraft hub.docker.com/eraftio/tinydb:latest /eraft/run-test.sh
