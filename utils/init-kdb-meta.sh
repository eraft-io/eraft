#!/bin/bash
set -xe

redis-cli -h 172.18.0.6 -p 12306 shardgroup join 1 172.18.0.10:8088,172.18.0.11:8089,172.18.0.12:8090

# for i in {1..1023}; do redis-cli -h 172.18.0.6 -p 12306 shardgroup move 1 $i; done

redis-cli -h 172.18.0.6 -p 12306 shardgroup move 1 0-1023
