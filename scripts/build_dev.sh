#!/bin/bash
set -xe

export PATH=$HOME/go/bin:/usr/local/go/bin:$PATH

cd /eraft && make
