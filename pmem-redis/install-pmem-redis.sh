#!/usr/bin/env bash

set -e

PMEM_REDIS_VERSION="7789bb18b673e3f79bd90992fcbd7aefadd368a6"

apt update -y && \
    apt install -y cmake gcc g++ autoconf automake libtool numactl tcl ndctl daxctl

git clone https://github.com/pmem/pmem-redis.git
cd pmem-redis
git checkout ${PMEM_REDIS_VERSION}

make USE_NVM=yes
