# KV Raft Key-Value Store Tests

**Test File**: `test/kvtest_main.cpp`

This test suite validates a linearizable KV store built on top of Raft, covering basic operations (3A) and snapshot-based recovery (3B). Corresponds to MIT 6.824 Lab 3.

All tests include a **Porcupine linearizability checker** that formally verifies whether the operation history is linearizable.

---

## 3A: Basic KV Operations

Validates linearizable semantics of the KV store under various failure conditions.

### Single-Client Tests

| Test Case | Cluster Size | What It Validates |
|-----------|-------------|-------------------|
| **Basic3A** | 1 client, 5 servers | Basic KV operations: serial Put/Append/Get with multiple rounds of crash/restart cycles |
| **Speed3A** | 1 client, 3 servers | Operation throughput: 1000 Append operations; per-op latency must be under 33 ms |

### Concurrency Tests

| Test Case | Cluster Size | What It Validates |
|-----------|-------------|-------------------|
| **Concurrent3A** | 5 clients, 5 servers | Concurrent clients: each client appends to its own key; verifies that concurrent writes do not conflict and are linearizable |
| **Unreliable3A** | 5 clients, 5 servers | Unreliable network + concurrency: concurrent operations remain linearizable over an unreliable network |
| **UnreliableOneKey3A** | 5 clients, 3 servers | Unreliable network, single key: 5 clients concurrently append to the same key; verifies safe concurrent mutation of shared state |

### Network Partition Tests

| Test Case | Cluster Size | What It Validates |
|-----------|-------------|-------------------|
| **OnePartition3A** | 5 servers | Single partition: the majority partition continues to serve requests; minority partition requests block; pending operations complete after the partition heals |
| **ManyPartitionsOneClient3A** | 1 client, 5 servers | Frequent partitions + single client: 3 rounds of random partitioning with data consistency verification after each round |
| **ManyPartitionsManyClients3A** | 5 clients, 5 servers | Frequent partitions + multiple clients: random partitions plus concurrent operations; verifies linearizability |

### Persistence + Fault Tolerance Tests

| Test Case | Cluster Size | What It Validates |
|-----------|-------------|-------------------|
| **PersistOneClient3A** | 1 client, 5 servers | Persistence + single client: 3 rounds of full cluster crash/restart; committed data is not lost |
| **PersistConcurrent3A** | 5 clients, 5 servers | Persistence + concurrent clients: concurrent operations remain correct across crash/restart cycles |
| **PersistConcurrentUnreliable3A** | 5 clients, 5 servers | Persistence + concurrency + unreliable network: crash, network unreliability, and concurrency combined |
| **PersistPartition3A** | 5 clients, 5 servers | Persistence + partitions: crash/restart and network partitions coexist |
| **PersistPartitionUnreliable3A** | 5 clients, 5 servers | Persistence + partitions + unreliable network: all three failure modes combined |
| **PersistPartitionUnreliableLinearizable3A** | 15 clients, 7 servers | Full linearizability under chaos: crashes, partitions, unreliable network, and random keys; 15 concurrent clients on 7 servers produce a linearizable history |

**Key Properties Verified**:
- Linearizability: every operation history is checked by the Porcupine model checker
- Duplicate detection: clients retry requests without causing duplicate application
- Idempotency: Put/Append applied exactly once even when the client retries
- Leader redirect: clients automatically retry on WrongLeader errors

---

## 3B: KV Operations with Snapshots

Validates the KV store when Raft log compaction (snapshotting) is enabled.

### Snapshot Mechanism Tests

| Test Case | Cluster Size | What It Validates |
|-----------|-------------|-------------------|
| **SnapshotRPC3B** | 3 servers | InstallSnapshot RPC: a lagging node that misses many log entries catches up via snapshot instead of replaying individual entries; verifies split-brain recovery |
| **SnapshotSize3B** | 3 servers | Snapshot size is reasonable: after 200 Put cycles, log size stays under 8×maxraftstate and snapshot size stays under 500 bytes |
| **Speed3B** | 1 client, 3 servers | Snapshot throughput: 1000 Append operations with snapshotting enabled; per-op latency must be under 33 ms |

### Snapshot + Recovery Tests

| Test Case | Cluster Size | What It Validates |
|-----------|-------------|-------------------|
| **SnapshotRecover3B** | 1 client, 5 servers | Snapshot + crash recovery: single client, crash/restart cycles with snapshots; data survives and cluster recovers |
| **SnapshotRecoverManyClients3B** | 20 clients, 5 servers | Snapshot + many clients: 20 concurrent clients with crash/restart; snapshot log trimming keeps up with load |
| **SnapshotUnreliable3B** | 5 clients, 5 servers | Snapshot + unreliable network: concurrent operations over an unreliable network with snapshotting |
| **SnapshotUnreliableRecover3B** | 5 clients, 5 servers | Snapshot + unreliable + crash: network unreliability and crash/restart with snapshotting |
| **SnapshotUnreliableRecoverConcurrentPartition3B** | 5 clients, 5 servers | Snapshot + all failure modes: snapshotting with concurrency, unreliable network, crash/restart, and partitions |
| **SnapshotUnreliableRecoverConcurrentPartitionLinearizable3B** | 15 clients, 7 servers | Full linearizability under chaos with snapshots: the most comprehensive test combining all failure modes with formal linearizability verification |

**Key Properties Verified**:
- Log size bounded: after snapshotting, Raft log size stays within 8×maxraftstate
- Snapshot size reasonable: snapshots are compact and do not grow unbounded
- InstallSnapshot correctness: lagging nodes can be brought up to date via snapshot transfer
- Crash recovery with snapshots: nodes restart from snapshot and continue serving correctly