[![Language](https://img.shields.io/badge/Language-Go-blue.svg)](https://golang.org/)
[![License](https://img.shields.io/badge/license-MIT-green)](https://opensource.org/licenses/MIT)

[中文](README_cn.md) | English

# eRaft: A Distributed Sharded KV Storage System

eRaft is a high-performance distributed key-value storage system implemented in Go. It features:
- **Consensus**: Raft algorithm for consistency and high availability.
- **Transport**: gRPC for efficient inter-node and client-server communication.
- **Storage**: LevelDB as the persistent storage engine.
- **Sharding**: Dynamic sharding with a dedicated configuration cluster.

## Build

To build all components, run:
```bash
make build
```
Binaries will be generated in the `output/` directory.

## Quick Start Guide

### Step 1: Start the Configuration Cluster (ShardCtrler)

The configuration cluster manages shard assignments. Start 3 nodes:

```bash
# Terminal 1
./output/shardctrlerserver -id 0 -cluster "localhost:50051,localhost:50052,localhost:50053" -db "data/sc0"

# Terminal 2
./output/shardctrlerserver -id 1 -cluster "localhost:50051,localhost:50052,localhost:50053" -db "data/sc1"

# Terminal 3
./output/shardctrlerserver -id 2 -cluster "localhost:50051,localhost:50052,localhost:50053" -db "data/sc2"
```

### Step 2: Start a ShardKV Group

A ShardKV group stores the actual data. Start a group with GID 100:

```bash
# Terminal 4
./output/shardkvserver -id 0 -gid 100 -cluster "localhost:6001,localhost:6002,localhost:6003" -ctrlers "localhost:50051,localhost:50052,localhost:50053" -db "data/skv100_0"

# Terminal 5
./output/shardkvserver -id 1 -gid 100 -cluster "localhost:6001,localhost:6002,localhost:6003" -ctrlers "localhost:50051,localhost:50052,localhost:50053" -db "data/skv100_1"

# Terminal 6
./output/shardkvserver -id 2 -gid 100 -cluster "localhost:6001,localhost:6002,localhost:6003" -ctrlers "localhost:50051,localhost:50052,localhost:50053" -db "data/skv100_2"
```

### Step 3: Register the ShardKV Group

Use the `shardctrlerclient` to join the group to the configuration cluster:

```bash
./output/shardctrlerclient join 100=localhost:6001,localhost:6002,localhost:6003
```

### Step 4: Data Operations

Now you can read and write data using the `shardkvclient`:

```bash
# Write data
./output/shardkvclient put mykey myvalue

# Read data
./output/shardkvclient get mykey

# Append data
./output/shardkvclient append mykey " extra"
```

### Cluster Monitoring

Check the status of all nodes in the cluster:

```bash
# ShardKV status
./output/shardkvclient status

# ShardCtrler status
./output/shardctrlerclient status
```
