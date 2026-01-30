# Sharding Mechanism

eRaft uses horizontal partitioning (sharding) to scale data storage and processing across multiple Raft groups.

## Shard Assignment
Data is divided into 10 logical shards. The `ShardCtrler` manages the mapping from `ShardID -> GroupID`. 

## Data Migration
When the cluster configuration changes (e.g., a new group joins), the system performs an automatic "live" migration:

1. **Detection**: Group leaders poll the `ShardCtrler` for new configurations.
2. **Pulling**: The new owner of a shard initiates a gRPC call to the previous owner to fetch the shard's data.
3. **Log Entry**: The received data is committed as an `InsertShards` command in the new owner's Raft log.
4. **Clean up**: After successful integration, a `DeleteShards` (GC) request is sent to the old owner to remove the stale data.

## Shard Statuses
- **Serving**: The shard is fully owned and available for operations.
- **Pulling**: The group is currently fetching data for this shard.
- **BePulling**: The group is waiting for another group to finish pulling this shard's data.
- **GCing**: The group is waiting to notify the previous owner to delete data.
