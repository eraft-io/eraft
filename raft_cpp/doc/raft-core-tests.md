# Raft Core Consensus Algorithm Tests

**Test File**: `test/test_main.cpp`

This test suite covers the core implementation of the Raft consensus algorithm, including leader election, log replication, persistence, and snapshotting. Corresponds to MIT 6.824 Lab 2A/2B/2C/2D.

---

## 2A: Leader Election

Validates the Raft leader election mechanism.

| Test Case | Cluster Size | What It Validates |
|-----------|-------------|-------------------|
| **InitialElection2A** | 3 servers | Initial election: a clean cluster elects a single leader, and no re-election occurs within the same term |
| **ReElection2A** | 3 servers | Re-election after network failure: disconnecting the leader triggers a new election; the old leader rejoins cleanly after reconnection |
| **ManyElections2A** | 7 servers | Many rounds of random elections: random disconnect/reconnect cycles over 10 rounds, each round producing exactly one leader |

**Key Properties Verified**:
- Leader uniqueness: at most one leader at any time
- Election timeout: a new election is triggered promptly after the leader is disconnected
- Monotonic terms: term numbers never decrease

---

## 2B: Log Replication

Validates Raft log replication and consistency guarantees.

| Test Case | Cluster Size | What It Validates |
|-----------|-------------|-------------------|
| **BasicAgree2B** | 3 servers | Basic log replication: 3 commands committed sequentially, all nodes agree on committed values |
| **FailAgree2B** | 3 servers | Agreement despite follower disconnection: a majority continues to commit while a follower is disconnected; the follower catches up after reconnection |
| **FailNoAgree2B** | 5 servers | No agreement when majority is lost: with 3 nodes disconnected, the leader cannot commit new entries; commits resume after recovery |
| **Rejoin2B** | 3 servers | Rejoin of a partitioned leader: entries from the old leader that were not replicated to a majority are discarded after it rejoins |
| **Backup2B** | 5 servers | Leader backs up quickly: the leader detects and corrects stale follower logs via incremental AppendEntries |

**Key Properties Verified**:
- Log consistency: all committed entries at the same index have the same content
- Majority quorum: an entry is committed only when acknowledged by a majority
- Conflict resolution: the leader uses prevLogIndex/prevLogTerm to detect and overwrite conflicting entries

---

## 2C: Persistence

Validates that servers recover correctly from persistent storage after restart.

| Test Case | Cluster Size | What It Validates |
|-----------|-------------|-------------------|
| **Persist12C** | 3 servers | Basic persistence: multiple restart cycles per node; committed data is not lost, and the cluster continues operating normally |
| **Figure82C** | 5 servers | Figure 8 extreme scenario: 1000 rounds of random crash/restart, simulating the complex log conflict described in the Raft paper Figure 8; final data is consistent across all nodes |
| **UnreliableAgree2C** | 5 servers | Agreement over unreliable network: 50 rounds of concurrent submissions with an unreliable network; all nodes converge after reliability is restored |

**Key Properties Verified**:
- State persistence: currentTerm, votedFor, and log[] are fully restored after restart
- Crash recovery: a crashed node recovers from persistent state and rejoins the cluster
- Unreliable network: message loss, delay, and reordering do not affect eventual consistency

---

## 2D: Snapshot

Validates the correctness of log compaction (snapshot) mechanism.

| Test Case | Cluster Size | What It Validates |
|-----------|-------------|-------------------|
| **SnapshotBasic2D** | 3 servers | Basic snapshots: log size stays bounded under 2000 bytes; no data is lost after compaction |
| **SnapshotInstall2D** | 3 servers | Install snapshots via disconnection: logs are compacted while a node is disconnected; the lagging node catches up via InstallSnapshot RPC upon reconnection |
| **SnapshotInstallUnreliable2D** | 3 servers | Install snapshots over unreliable network: snapshot transfer completes correctly even with an unreliable network |
| **SnapshotInstallCrash2D** | 3 servers | Install snapshots after crash: a node recovers from crash and catches up via InstallSnapshot |
| **SnapshotInstallUnCrash2D** | 3 servers | Install snapshots with unreliable network + crash: the most complex snapshot scenario, combining network unreliability with node crashes |

**Key Properties Verified**:
- Log truncation: log size stays below the threshold after snapshotting
- InstallSnapshot RPC: lagging nodes catch up via snapshot transfer rather than replaying individual log entries
- Snapshot consistency: state restored from a snapshot is identical to applying all log entries individually