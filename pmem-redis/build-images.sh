#!/bin/bash

BUILDER_IMAGE="eraft/eraft_pmem_redis"

docker build -f Dockerfile -t ${BUILDER_IMAGE} .
