[![License](https://img.shields.io/badge/license-MIT-green)](https://opensource.org/licenses/MIT)

# eRaft: A Distributed Sharded KV Storage System

A C++17 library implementing the Raft consensus algorithm and a sharded key-value store on top of it.

## Modules

| Module | Source | Description |
|--------|--------|-------------|
| **raft** | `raft_cpp/src/raft.cpp` | Core Raft consensus: leader election, log replication, snapshotting, persistence |
| **kvraft** | `raft_cpp/src/kvraft_server.cpp` | Fault-tolerant KV store on Raft, with client deduplication and linearizable semantics |
| **shardctrler** | `raft_cpp/src/shardctrler.cpp` | Configuration cluster managing dynamic shard-to-group assignment |
| **shardkv** | `raft_cpp/src/shardkv.cpp` | Sharded KV store with cross-group data migration, garbage collection, and concurrent reconfiguration |

## Architecture

```
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│  ShardKVClerk │  │  KVClerk     │  │  SCClerk     │  ← Client
└──────┬───────┘  └──────┬───────┘  └──────┬───────┘
       │                 │                 │
┌──────▼───────┐  ┌──────▼───────┐  ┌──────▼───────┐
│   ShardKV    │  │   KVServer   │  │ ShardCtrler  │  ← Service
└──────┬───────┘  └──────┬───────┘  └──────┬───────┘
       │                 │                 │
┌──────▼─────────────────▼─────────────────▼───────┐
│                     Raft                          │  ← Consensus
└───────────────────────────────────────────────────┘
```

## Key Features

- **Header-only public API** — `include/raft/*.h` exposes all types and interfaces
- **In-memory peer transport** — `InMemPeer` / `InMemRaftPeer` for fast single-process testing
- **gRPC support** — `grpc_server.cpp` / `grpc_client.cpp` for real network deployment
- **BlockingQueue** — lock-based notify channel for coordinating Raft applier with service-layer `Execute()`
- **Snapshot + persistence** — `Persister` with `readPersist()` / `persist()` for crash recovery

## Build & Test

```bash
cd raft_cpp
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.logicalcpu)

# Run all tests
./raft_test           # Raft: 12 tests
./kvraft_test         # KV Raft: 4 tests
./shardctrler_test    # ShardCtrler: 4 tests
./shardkv_test        # ShardKV: 6 tests
```

## Dependencies

- C++17 compiler (Clang / GCC)
- CMake ≥ 3.16
- gRPC + Protobuf
- Google Test
- RocksDB

macOS:
```bash
brew install cmake grpc protobuf googletest rocksdb
```