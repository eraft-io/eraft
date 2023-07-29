#!/bin/bash
set -xe

redis-cli -h 172.18.0.6 -p 12306 shardgroup query

sleep 1 # test mode raft interval is 1s

redis-cli -h 172.18.0.6 -p 12306 info
redis-cli -h 172.18.0.6 -p 12306 set a h
redis-cli -h 172.18.0.6 -p 12306 set b e
redis-cli -h 172.18.0.6 -p 12306 set c l
redis-cli -h 172.18.0.6 -p 12306 set d l
redis-cli -h 172.18.0.6 -p 12306 set e o

# sleep 1

redis-cli -h 172.18.0.6 -p 12306 get a
redis-cli -h 172.18.0.6 -p 12306 get b
redis-cli -h 172.18.0.6 -p 12306 get c
redis-cli -h 172.18.0.6 -p 12306 get d
redis-cli -h 172.18.0.6 -p 12306 get e

redis-cli -h 172.18.0.6 -p 12306 del e

# sleep 1

redis-cli -h 172.18.0.6 -p 12306 get e
