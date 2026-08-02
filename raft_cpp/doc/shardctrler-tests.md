# ShardCtrler Configuration Management Tests

**Test File**: `test/sctest_main.cpp`

This test suite validates the ShardCtrler (shard controller), which manages the cluster configuration — which shard is assigned to which group. Corresponds to MIT 6.824 Lab 4A.

---

## Test Cases

### TestBasic

A comprehensive test covering all basic shard controller operations.

| Phase | What It Validates |
|-------|-------------------|
| **Basic join/leave** | Join group 1 (`gid=1`), verify shards are assigned; join group 2 (`gid=2`), verify shards are balanced between two groups; verify group server addresses are stored correctly; leave group 1, verify all shards move to group 2; leave group 2, verify all shards go to `gid=0` (unassigned) |
| **Historical queries** | Shut down each server one at a time and query all historical configs by number; verifies that the full configuration history is preserved across restarts |
| **Move** | Join two groups (503, 504); manually move each shard — first half to group 503, second half to group 504; verify each `Move` call increments `Config.Num`; verify final shard assignment matches expectations |
| **Concurrent join/leave** | 10 concurrent clients each join their own group and a temporary group, then leave the temporary group; verify final configuration contains exactly the 10 intended groups |
| **Minimal transfers after joins** | Join 5 new groups; verify that shards already assigned to existing groups are not moved to other groups (minimal disruption) |
| **Minimal transfers after leaves** | Leave the 5 groups just added; verify that shards already assigned to remaining groups are not moved |

**Key Properties Verified**:
- Shard assignment: all shards are assigned to valid groups
- Shard balance: the difference between the most-loaded and least-loaded groups is at most 1
- Config history: all historical configs are preserved and queryable
- Move semantics: `Move` increments config number and enforces the specified shard placement
- Minimal transfers: rebalancing moves only the minimum number of shards (equalization principle)

---

### TestMulti

Validates multi-group batch operations on the shard controller.

| Phase | What It Validates |
|-------|-------------------|
| **Multi-group join** | Join two groups (`gid=1,2`) in a single `Join` call; verify shards are balanced across both groups; join a third group (`gid=3`), verify balanced distribution across three groups |
| **Multi-group leave** | Leave groups 1 and 3 in a single `Leave` call; verify all shards move to group 2; leave group 2, verify all shards go to `gid=0` |
| **Concurrent multi join/leave** | 10 concurrent clients each batch join 3 groups and batch leave 2 of them; verify final configuration contains exactly the 10 intended groups |
| **Minimal transfers after multi-joins** | Batch join 5 groups; verify shards already assigned to existing groups are not moved |
| **Minimal transfers after multi-leaves** | Batch leave the 5 groups; verify shards already assigned to remaining groups are not moved |

**Key Properties Verified**:
- Batch operations: multiple groups can be joined or left in a single Raft command
- Deterministic rebalancing: given the same input, the same shard assignment is produced
- Minimal disruption: only the absolute minimum shards are moved during rebalancing