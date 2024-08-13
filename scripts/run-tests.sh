#!/bin/bash
set -xe

# run test exe
/eraft/build/example/eraftkv-ctl 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090 add_group 1 172.18.0.10:8088,172.18.0.11:8089,172.18.0.12:8090
/eraft/build/example/eraftkv-ctl 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090 query_groups
/eraft/build/example/eraftkv-ctl 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090 set_slot 1 0-9
/eraft/build/example/eraftkv-ctl 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090 run_bench 1000
