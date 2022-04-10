#!/bin/bash

set -xe

export LD_LIBRARY_PATH=/usr/local/lib
/eraft/build_/cmd/pmemkv_redisd /eraft/etc/pmem_redis.toml