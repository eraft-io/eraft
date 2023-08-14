#!/bin/bash
set -xe

redis-cli -h 172.18.0.6 -p 12306 shardgroup query

sleep 1 # test mode raft interval is 1s

redis-cli -h 172.18.0.6 -p 12306 info
cat /eraft/utils/test_commands.txt | redis-cli -h 172.18.0.6 -p 12306