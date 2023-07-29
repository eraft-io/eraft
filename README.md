# ERaftKDB

ERaftKDB is a distributed database that supports the Redis RESP protocol, and uses ERaftKV as the distributed storage layer.

![eraft-kdb](doc/eraft-kdb.png)

## Redis Command Support Plan

| Command    | Status |
| -------- | ------- |
| get  |  [x] |
| set  | [x]   |
| del  | [x]   |
| scan  | [ ]   |


## ERaftKV

ERaftKV is a persistent distributed KV storage system, uses the Raft protocol to ensure data consistency, At the same time, it supports sharding to form multi shard large-scale data storage clusters.

## ERaftKV Features
- Strong and consistent data storage ensures secure and reliable data persistence in distributed systems.
- Support KV data type operations, including PUT, GET, DEL, and SCAN operations on keys. When users operate on cluster data, they must ensure the persistence of the operation and the sequential consistency of reading and writing.
- Dynamically configure the cluster, including adding and deleting nodes, adding and deleting cluster sharding configurations, including which keyrange the cluster sharding is responsible for.
- Support for snapshot taking with the raft to compress and merge logs. During the snapshot, it is required to not block data read and write.
- Support switching to a specifying leader.
- Raft elections support PreVote, and newly added nodes do not participate in the election by tracking data to avoid triggering unnecessary elections.
- Raft read optimization: adding a read queue ensures that the leader node returns a read request after submitting it, and unnecessary logs are not written.
- Support data migration, multi-shard scale out.

# Getting Started

## Build 

Execute follower build command on the machine with docker installed.

```
sudo make build-dev
```

## Run demo in docker

- step 1, create docker sub net

```
sudo make create-net
```

command output
```
docker network create --subnet=172.18.0.0/16 mytestnetwork
f57ad3d454f27f4b84efca3ce61bf4764bd30ce3d4971b85477daf05c6ae28a3
```

- step 2, run cluster in shard mode

```
sudo make run-demo
```
command output
```
docker run --name kvserver-node1 --network mytestnetwork --ip 172.18.0.10 -d --rm -v /home/colin/eraft:/eraft eraft/eraftkv:v0.0.6 /eraft/build/eraftkv 0 /tmp/kv_db0 /tmp/log_db0 172.18.0.10:8088,172.18.0.11:8089,172.18.0.12:8090
bef74b85fcf9c0a2dedb15399b1f53010791e329f0c60d69fcd097e0843cbb86
sleep 2
docker run --name kvserver-node2 --network mytestnetwork --ip 172.18.0.11 -d --rm -v /home/colin/eraft:/eraft eraft/eraftkv:v0.0.6 /eraft/build/eraftkv 1 /tmp/kv_db1 /tmp/log_db1 172.18.0.10:8088,172.18.0.11:8089,172.18.0.12:8090
333c02093fcb8c974cc1dc491fc7d2e19f474e3fda354fc130cf6be6d8920c85
docker run --name kvserver-node3 --network mytestnetwork --ip 172.18.0.12 -d --rm -v /home/colin/eraft:/eraft eraft/eraftkv:v0.0.6 /eraft/build/eraftkv 2 /tmp/kv_db2 /tmp/log_db2 172.18.0.10:8088,172.18.0.11:8089,172.18.0.12:8090
9856291dd34776cea94ab957780f7a244cb387bd0d74388b5a9d440175d6d28e
sleep 1
docker run --name metaserver-node1 --network mytestnetwork --ip 172.18.0.2 -d --rm -v /home/colin/eraft:/eraft eraft/eraftkv:v0.0.6 /eraft/build/eraftmeta 0 /tmp/meta_db0 /tmp/log_db0 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090
09f9f12bc74212d1ae09a89bfecbc5a991f1b46cd9e8ba43fc278f775dd6176d
sleep 3
docker run --name metaserver-node2 --network mytestnetwork --ip 172.18.0.3 -d --rm -v /home/colin/eraft:/eraft eraft/eraftkv:v0.0.6 /eraft/build/eraftmeta 1 /tmp/meta_db1 /tmp/log_db1 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090
3b98b3f317e834263ddb81c0bc5b245ac31b69cd47f495415a3d70e951c13900
docker run --name metaserver-node3 --network mytestnetwork --ip 172.18.0.4 -d --rm -v /home/colin/eraft:/eraft eraft/eraftkv:v0.0.6 /eraft/build/eraftmeta 2 /tmp/meta_db2 /tmp/log_db2 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090
10269f84d95e9f82f75d3c60f0d7b0dc0efe5efe643366e615b7644fb8851f04
sleep 16
docker run --name vdbserver-node --network mytestnetwork --ip 172.18.0.6 -it --rm -v /home/colin/eraft:/eraft eraft/eraftkv:v0.0.6 /eraft/build/eraft-kdb 172.18.0.6:12306 172.18.0.2:8088,172.18.0.3:8089,172.18.0.4:8090
run server success!
```

- step 3, run eraft kdb tests

```
sudo make init-kdb-meta
sudo make run-kdb-tests
```
command output

```
chmod +x utils/run-kdb-tests.sh
docker run --name vdbserver-node-tests --network mytestnetwork --ip 172.18.0.9 -it --rm -v /Users/colin/Documents/eraft:/eraft eraft/eraftkv:v0.0.6 /eraft/utils/run-kdb-tests.sh
+ redis-cli -h 172.18.0.6 -p 12306 shardgroup query
1) "shardgroup"
2) "1"
3) "servers"
4) "0"
5) "172.18.0.10:8088"
6) "1"
7) "172.18.0.11:8089"
8) "2"
9) "172.18.0.12:8090"
+ redis-cli -h 172.18.0.6 -p 12306 shardgroup join 1 172.18.0.10:8088,172.18.0.11:8089,172.18.0.12:8090
OK
+ redis-cli -h 172.18.0.6 -p 12306 shardgroup move 1 0-1023
OK
+ sleep 1
+ redis-cli -h 172.18.0.6 -p 12306 info
meta server:
server_id: 0,server_address: 172.18.0.2:8088,status: Running,Role: Leader
meta server:
server_id: 1,server_address: 172.18.0.3:8089,status: Running,Role: Follower
meta server:
server_id: 2,server_address: 172.18.0.4:8090,status: Running,Role: Follower
+ redis-cli -h 172.18.0.6 -p 12306 set a h
OK
+ redis-cli -h 172.18.0.6 -p 12306 set b e
OK
+ redis-cli -h 172.18.0.6 -p 12306 set c l
OK
+ redis-cli -h 172.18.0.6 -p 12306 set d l
OK
+ redis-cli -h 172.18.0.6 -p 12306 set e o
OK
+ sleep 1
+ redis-cli -h 172.18.0.6 -p 12306 get a
"h"
+ redis-cli -h 172.18.0.6 -p 12306 get b
"e"
+ redis-cli -h 172.18.0.6 -p 12306 get c
"l"
+ redis-cli -h 172.18.0.6 -p 12306 get d
"l"
+ redis-cli -h 172.18.0.6 -p 12306 get e
"o"
+ redis-cli -h 172.18.0.6 -p 12306 get nil_test
(nil)
```

- step 4, clean all
```
sudo make stop-demo
sudo make rm-net
```

## Building and run test using [GitHub Actions](https://github.com/features/actions)

All you need to do is submit a Pull Request to our repository, and all compile, build, and testing will be automatically executed.

You can check the [ERatfKVGitHubActions](https://github.com/eraft-io/eraft/actions) See the execution process of code build tests.

## Building on your local machine.

To compile eraftkv, you will need:
- Docker

To build:
```
make gen-protocol-code
make build-dev
```

Run test:
```
make tests
```

If you want to build image youtself
```
make image
```

# Documentation
[ERaftKV Documentation](doc/eraft-kdb.md)

# Contributing

You can quickly participate in development by following the instructions in [CONTROLUTING.md](https://github.com/eraft-io/eraft/blob/master/CONTRIBUTING.md)
