#!/bin/bash
set -xe

if [ ! -d "data" ]; then mkdir data; fi;
if [ ! -d "logs" ]; then mkdir logs; fi;

nohup ./build/example/eraftkv -svr_id 0 -kv_db_path ./data/kv_db0 -log_db_path ./data/log_db0 -snap_db_path ./data/snap_db0 -peer_addrs 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 -log_file_path ./logs/eraftkv-1.log -monitor_port 12306 &
nohup ./build/example/eraftkv -svr_id 1 -kv_db_path ./data/kv_db1 -log_db_path ./data/log_db1 -snap_db_path ./data/snap_db1 -peer_addrs 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 -log_file_path ./logs/eraftkv-2.log -monitor_port 12307 &
nohup ./build/example/eraftkv -svr_id 2 -kv_db_path ./data/kv_db2 -log_db_path ./data/log_db2 -snap_db_path ./data/snap_db2 -peer_addrs 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 -log_file_path ./logs/eraftkv-3.log -monitor_port 12308 &

nohup ./build/example/eraftmeta -svr_id 0 -kv_db_path ./data/meta_db0 -log_db_path ./data/meta_log_db0 -peer_addrs 127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090 -monitor_port 12309 &
nohup ./build/example/eraftmeta -svr_id 1 -kv_db_path ./data/meta_db1 -log_db_path ./data/meta_log_db1 -peer_addrs 127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090 -monitor_port 12310 &
nohup ./build/example/eraftmeta -svr_id 2 -kv_db_path ./data/meta_db2 -log_db_path ./data/meta_log_db2 -peer_addrs 127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090 -monitor_port 12311 &

sleep 3

./build/example/eraftkv-ctl 127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090 add_group 1 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090
./build/example/eraftkv-ctl 127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090 query_groups
./build/example/eraftkv-ctl 127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090 set_slot 1 0-9
./build/example/eraftkv-ctl 127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090 run_bench 100
