#!/bin/bash
set -xe

redis-cli -h 172.18.0.6 -p 12306 info
