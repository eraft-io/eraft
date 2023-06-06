# ERaftKV 

ERaftKV is a persistent distributed KV storage system, uses the Raft protocol to ensure data consistency, At the same time, it supports sharding to form multi shard large-scale data storage clusters.

# Features
- Strong and consistent data storage ensures secure and reliable data persistence in distributed systems.
- Support KV data type operations, including PUT, GET, DEL, and SCAN operations on keys. When users operate on cluster data, they must ensure the persistence of the operation and the sequential consistency of reading and writing.
- Dynamically configure the cluster, including adding and deleting nodes, adding and deleting cluster sharding configurations, including which keyrange the cluster sharding is responsible for.
- Support for snapshot taking with the raft to compress and merge logs. During the snapshot, it is required to not block data read and write.
- Support switching to a specifying leader.
- Raft elections support PreVote, and newly added nodes do not participate in the election by tracking data to avoid triggering unnecessary elections.
- Raft read optimization: adding a read queue ensures that the leader node returns a read request after submitting it, and unnecessary logs are not written.
- Support data migration, multi-shard scale out.

# Getting Started

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

## Run Demo

- how to set up demo cluster

```
./build/eraftkv 0 /tmp/kv_db0 /tmp/log_db0 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090
./build/eraftkv 1 /tmp/kv_db1 /tmp/log_db1 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090
./build/eraftkv 2 /tmp/kv_db2 /tmp/log_db2 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090

```

- how to join an new node?

```
./build/eraftkv 3 /tmp/kv_db4 /tmp/log_db4 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090,127.0.0.1:8091

./build/eraftkv-ctl addnode 3 127.0.0.1:8091
```

## example usage

- put kv
```
./eraftkv-ctl [leader_address] put [key] [value]
```
- get kv
```
./eraftkv-ctl [leader_address] get [key]
```

- addnode to raft group
```
./eraftkv-ctl [leader_address] addnode [node id] [node address]
```
- remove node from raft group
```
./eraftkv-ctl [leader_address] removenode [node id]
```

- run kv benchmark
```
./eraftkv-ctl [leader_address] bench [key size] [value size] [op count]
```

# Contributing

You can quickly participate in development by following the instructions in [CONTROLUTING.md](https://github.com/eraft-io/eraft/blob/master/CONTRIBUTING.md)
