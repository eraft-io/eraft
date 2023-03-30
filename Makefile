# Makefile
default: image

BUILDER_IMAGE := $(or $(BUILDER_IMAGE),hub.docker.com/eraftio/eraftkv)

image:
	docker build -f Dockerfile -t $(BUILDER_IMAGE) .
