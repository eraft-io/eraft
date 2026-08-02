# ShardKV Sharded Key-Value Store Tests

**Test File**: `test/skvtest_main.cpp`

This test suite validates the sharded KV store (ShardKV), which partitions data across multiple replica groups and supports dynamic configuration changes. Corresponds to MIT 6.824 Lab 4B.

The test framework sets up 3 replica groups (`gid=100,101,102`), each with 3 Raft servers, plus 3 ShardCtrler servers for configuration management.

---

## Test Cases

### TestStaticShards

Validates static 2-way sharding without shard movement.

| Phase | What It Validates |
|-------|-------------------|
| **Setup** | Join groups 0 and 1; wait for configuration propagation; the 10 shards are statically partitioned between the two groups |
| **Basic writes** | Put 10 key-value pairs; verify all Get operations return the correct values |
| **Group failure** | Shut down group 1; verify that approximately half the keys become unavailable (the other half remain served by group 0) |
| **Group recovery** | Restart group 1; verify all keys are accessible again and data is intact |

**Key Properties Verified**:
- Shard-based data partitioning: keys are distributed across groups based on hash-based shard assignment
- Fault isolation: when one group fails, only the shards owned by that group become unavailable
- Data durability: shard data survives group restart

---

### TestJoinLeave

Validates data migration when groups join and leave.

| Phase | What It Validates |
|-------|-------------------|
| **Write to single group** | Join group 0; write 10 keys; verify all reads succeed |
| **Join group 1** | Add group 1; verify all existing keys are still accessible; append new data to each key; verify shards are migrated from group 0 to group 1 as needed |
| **Leave group 0** | Remove group 0; verify all keys are still accessible; append new data to each key; wait for shard transfer to complete |
| **Shutdown departed group** | Shut down group 0; verify all keys remain accessible via the remaining group 1 |

**Key Properties Verified**:
- Shard migration: when a group joins, shards are rebalanced; when a group leaves, its shards are transferred to remaining groups
- Data integrity during migration: keys remain readable and writable throughout configuration changes
- No data loss: data originally on group 0 is accessible after group 0 leaves

---

### TestSnapshot

Validates snapshot-based persistence combined with configuration changes.

| Phase | What It Validates |
|-------|-------------------|
| **Write baseline** | Join group 0; write 30 keys; verify all reads |
| **First reconfiguration** | Join groups 1 and 2, leave group 0; append data to all keys; verify consistency after migration |
| **Second reconfiguration** | Leave group 1, join group 0; append data to all keys; verify consistency after another round of migration |
| **Full cluster restart** | Shut down all 3 groups; restart all 3 groups; verify all 30 keys are intact with all appended data |

**Key Properties Verified**:
- Snapshot + configuration changes: snapshots work correctly across multiple rounds of join/leave
- Full cluster restart recovery: after all nodes crash and restart, data is fully recovered from snapshots
- Data accumulates correctly: all append operations across reconfigurations contribute to the final value

---

### TestMissChange

Validates that servers that miss configuration changes can catch up when they restart.

| Phase | What It Validates |
|-------|-------------------|
| **Write baseline** | Join group 0; write 10 keys |
| **Shutdown servers during reconfig** | Join group 1; shut down server 0 of each group; then join group 2, leave groups 1 and 0; wait for migrations to complete on remaining servers; verify all reads succeed; append new data |
| **Restart and verify** | Join group 1; restart the 3 shutdown servers; verify all keys are intact; append new data |
| **Second round with different servers** | Shut down server 1 of each group; join group 0, leave group 2; wait for migrations; restart shutdown servers; verify all keys and appends preserved |

**Key Properties Verified**:
- Missed configuration recovery: a server that was down during configuration changes catches up after restart
- Migration correctness with partial membership: shard migration works even when some servers in each group are down
- Two-phase miss-and-recover: the same pattern works in a second round with different servers

---

### TestConcurrent1

Validates concurrent writes alongside configuration changes with snapshotting enabled.

| Phase | What It Validates |
|-------|-------------------|
| **Concurrent writers** | 10 background threads continuously append to 10 keys while configuration changes happen |
| **Rapid reconfiguration** | Join group 1, join group 2, leave group 0; then shut down all groups, leave group 2; restart all groups |
| **More reconfiguration** | Join group 0, leave group 1, join group 1 |
| **Final verification** | Stop all writers; verify each key's value matches the expected accumulated append result |

**Key Properties Verified**:
- Concurrent writes + reconfiguration: appends are not lost during configuration changes
- Snapshotting under load: snapshots are taken correctly while writes are ongoing
- Correctness under chaos: all append operations are reflected in the final state

---

### TestConcurrent2

Validates more aggressive concurrent writes and configuration changes without snapshotting.

| Phase | What It Validates |
|-------|-------------------|
| **Setup** | Join all 3 groups; write 10 keys |
| **Concurrent writers** | 10 background threads append to 10 keys |
| **Multiple reconfiguration cycles** | Leave groups 0 and 2; join groups 0 and 2; leave group 1; join group 1; leave groups 0 and 2 |
| **Group restart** | Shut down groups 1 and 2; restart groups 1 and 2 |
| **Final verification** | Stop all writers; verify each key's value matches the expected accumulated append result |

**Key Properties Verified**:
- Aggressive reconfiguration: multiple rapid join/leave cycles do not corrupt data
- Restart after reconfiguration: groups restarted after reconfiguration serve correct data
- Append ordering: within each key, append operations are applied in the correct order