#!/bin/bash
set -xe

redis-cli -h 172.18.0.6 -p 12306 shardgroup query
redis-cli -h 172.18.0.6 -p 12306 shardgroup join 1 172.18.0.10:8088,172.18.0.11:8089,172.18.0.12:8090

for i in {1..1023}; do redis-cli -h 172.18.0.6 -p 12306 shardgroup move 1 $i; done

sleep 1 # test mode raft interval is 1s

redis-cli -h 172.18.0.6 -p 12306 info
redis-cli -h 172.18.0.6 -p 12306 set a h
redis-cli -h 172.18.0.6 -p 12306 set b e
redis-cli -h 172.18.0.6 -p 12306 set c l
redis-cli -h 172.18.0.6 -p 12306 set d l
redis-cli -h 172.18.0.6 -p 12306 set e o

sleep 1 # test mode raft interval is 1s

redis-cli -h 172.18.0.6 -p 12306 get a
redis-cli -h 172.18.0.6 -p 12306 get b
redis-cli -h 172.18.0.6 -p 12306 get c
redis-cli -h 172.18.0.6 -p 12306 get d
redis-cli -h 172.18.0.6 -p 12306 get e
